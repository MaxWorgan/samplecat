#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <gtk/gtk.h>
#include "debug/debug.h"
#include "typedefs.h"
#include "support.h"
#include "audio_decoder/ad_plugin.h"

#ifdef HAVE_FFMPEG

#include "ffcompat.h"

typedef struct {
  AVFormatContext* formatContext;
  AVCodecContext*  codecContext;
  AVCodec*         codec;
  AVPacket         packet;
  int              audioStream;
  int              pkt_len;
  uint8_t*         pkt_ptr;

  int16_t          m_tmpBuffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
  int16_t*         m_tmpBufferStart;
  unsigned long    m_tmpBufferLen;

  int64_t          decoder_clock;
  int64_t          output_clock;
  int64_t          seek_frame;
  unsigned int     samplerate;
  unsigned int     channels;
  int64_t          length;
} ffmpeg_audio_decoder;


static gboolean
ad_metadata_array_set_tag_postion(GPtrArray* tags, const char* tag_name, int pos)
{
	// move a metadata item up (can only be up) to the specified position.
	// returns true if the tag was found.

	if((tags->len < 4) || (pos >= tags->len - 2)) return false;
	pos *= 2;

	char** data = (char**)tags->pdata;
	int i; for(i=pos;i<tags->len;i+=2){
		if(!strcmp(data[i], tag_name)){
			const char* artist[] = {data[i], (const char*)data[i+1]};

			int j; for(j=i;j>=pos;j-=2){
				data[j    ] = data[j - 2    ];
				data[j + 1] = data[j - 2 + 1];
			}
			data[pos    ] = (char*)artist[0];
			data[pos + 1] = (char*)artist[1];

			return true;
		}
	}
	return false;
}


int ad_info_ffmpeg(void *sf, struct adinfo *nfo) {
	ffmpeg_audio_decoder *priv = (ffmpeg_audio_decoder*) sf;
	if (!priv) return -1;
	if (nfo) {
		nfo->sample_rate = priv->samplerate;
		nfo->channels    = priv->channels;
		nfo->frames      = priv->length;
		if (!nfo->sample_rate) return -1;
		nfo->length      = (nfo->frames * 1000) / nfo->sample_rate;
		nfo->bit_rate    = priv->formatContext->bit_rate;
		nfo->bit_depth   = 0;
		nfo->meta_data   = NULL;

		GPtrArray* tags = g_ptr_array_new_full(32, g_free);

		AVDictionaryEntry *tag = NULL;
		// Tags in container
		while ((tag = av_dict_get(priv->formatContext->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
			dbg(2, "FTAG: %s=%s", tag->key, tag->value);
			g_ptr_array_add(tags, g_utf8_strdown(tag->key, -1));
			g_ptr_array_add(tags, g_strdup(tag->value));
		}
		// Tags in stream
		tag = NULL;
		AVStream *stream = priv->formatContext->streams[priv->audioStream];
		while ((tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
			dbg(2, "STAG: %s=%s", tag->key, tag->value);
			g_ptr_array_add(tags, g_utf8_strdown(tag->key, -1));
			g_ptr_array_add(tags, g_strdup(tag->value));
		}

		while ((tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
			g_ptr_array_add(tags, g_utf8_strdown(tag->key, -1));
			g_ptr_array_add(tags, g_strdup(tag->value));
		}

		if(tags->len){
			// sort tags
			char* order[] = {"artist", "title", "album", "track", "date"};
			int p = 0;
			int i; for(i=0;i<G_N_ELEMENTS(order);i++) if(ad_metadata_array_set_tag_postion(tags, order[i], p)) p++;

			nfo->meta_data = tags;
		}else
			g_ptr_array_free(tags, true);
	}
	return 0;
}

void *ad_open_ffmpeg(const char *fn, struct adinfo *nfo) {
  ffmpeg_audio_decoder *priv = (ffmpeg_audio_decoder*) calloc(1, sizeof(ffmpeg_audio_decoder));
  
  priv->m_tmpBufferStart=NULL;
  priv->m_tmpBufferLen=0;
  priv->decoder_clock=priv->output_clock=priv->seek_frame=0; 
  priv->packet.size=0; priv->packet.data=NULL;

  if (avformat_open_input(&priv->formatContext, fn, NULL, NULL) < 0) {
    dbg(1, "ffmpeg is unable to open file '%s'.", fn);
    free(priv); return(NULL);
  }

  if (avformat_find_stream_info(priv->formatContext, NULL) < 0) {
    avformat_close_input(&priv->formatContext);
    dbg(1, "av_find_stream_info failed");
    free(priv); return(NULL);
  }

  priv->audioStream = -1;
  int i;
  for (i=0; i<priv->formatContext->nb_streams; i++) {
    if (priv->formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
      priv->audioStream = i;
      break;
    }
  }
  if (priv->audioStream == -1) {
    dbg(1, "No Audio Stream found in file");
    avformat_close_input(&priv->formatContext);
    free(priv); return(NULL);
  }

  priv->codecContext = priv->formatContext->streams[priv->audioStream]->codec;
  priv->codec        = avcodec_find_decoder(priv->codecContext->codec_id);

  if (priv->codec == NULL) {
    avformat_close_input(&priv->formatContext);
    dbg(1, "Codec not supported by ffmpeg");
    free(priv); return(NULL);
  }
  if (avcodec_open2(priv->codecContext, priv->codec, NULL) < 0) {
    dbg(1, "avcodec_open failed" );
    free(priv); return(NULL);
  }

  dbg(2, "ffmpeg - audio tics: %i/%i [sec]", priv->formatContext->streams[priv->audioStream]->time_base.num, priv->formatContext->streams[priv->audioStream]->time_base.den);

  int64_t len = priv->formatContext->duration - priv->formatContext->start_time;

  priv->formatContext->flags |= AVFMT_FLAG_GENPTS;
  priv->formatContext->flags |= AVFMT_FLAG_IGNIDX;

  priv->samplerate = priv->codecContext->sample_rate;
  priv->channels   = priv->codecContext->channels;
  priv->length     = (int64_t)(len * priv->samplerate / AV_TIME_BASE);

  if (ad_info_ffmpeg((void*)priv, nfo)) {
    dbg(1, "invalid file info (sample-rate==0)");
    free(priv); return(NULL);
  }

  dbg(2, "ffmpeg - %s", fn);
  if (nfo) 
    dbg(2, "ffmpeg - sr:%i c:%i d:%"PRIi64" f:%"PRIi64, nfo->sample_rate, nfo->channels, nfo->length, nfo->frames);

  return (void*) priv;
}

int ad_close_ffmpeg(void *sf) {
  ffmpeg_audio_decoder *priv = (ffmpeg_audio_decoder*) sf;
  if (!priv) return -1;
  avcodec_close(priv->codecContext);
  avformat_close_input(&priv->formatContext);
  free(priv);
  return 0;
}

void int16_to_float(int16_t *in, float *out, int num_channels, int num_samples, int out_offset) {
  int i,ii;
  for (i=0;i<num_samples;i++) {
    for (ii=0;ii<num_channels;ii++) {
     out[(i+out_offset)*num_channels+ii]= (float) in[i*num_channels+ii]/ 32768.0;
    }
  }
}

ssize_t ad_read_ffmpeg(void *sf, float* d, size_t len) {
  ffmpeg_audio_decoder *priv = (ffmpeg_audio_decoder*) sf;
  if (!priv) return -1;
  size_t frames = len / priv->channels;

  int written = 0;
  int ret = 0;
  while (ret >= 0 && written < frames) {
    dbg(3, "loop: %i/%i (bl:%lu)", written, frames, priv->m_tmpBufferLen);
    if (priv->seek_frame == 0 && priv->m_tmpBufferLen > 0) {
      int s = MIN(priv->m_tmpBufferLen / priv->channels, frames - written);
      int16_to_float(priv->m_tmpBufferStart, d, priv->channels, s , written);
      written += s;
      priv->output_clock += s;
      s = s * priv->channels;
      priv->m_tmpBufferStart += s;
      priv->m_tmpBufferLen -= s;
      ret = 0;
    } else {
      priv->m_tmpBufferStart = priv->m_tmpBuffer;
      priv->m_tmpBufferLen = 0;

      if (!priv->pkt_ptr || priv->pkt_len < 1) {
        if (priv->packet.data) av_free_packet(&priv->packet);
        ret = av_read_frame(priv->formatContext, &priv->packet);
        if (ret < 0) { dbg(2, "reached end of file."); break; }
        priv->pkt_len = priv->packet.size;
        priv->pkt_ptr = priv->packet.data;
      }

      if (priv->packet.stream_index != priv->audioStream) {
        priv->pkt_ptr = NULL;
        continue;
      }

      /* decode all chunks in packet */
      int data_size= AVCODEC_MAX_AUDIO_FRAME_SIZE;
#if 0 // TODO  ffcompat.h -- this works but is not optimal (channels may not be planar/interleaved)
      AVFrame avf; // TODO statically allocate
      memset(&avf, 0, sizeof(AVFrame)); // not sure if that is needed
      int got_frame = 0;
ret = avcodec_decode_audio4(priv->codecContext, &avf, &got_frame, &priv->packet);
      data_size = avf.linesize[0];
      memcpy(priv->m_tmpBuffer, avf.data[0], avf.linesize[0] * sizeof(uint8_t));
#else // this was deprecated in LIBAVCODEC_VERSION_MAJOR 53
      ret = avcodec_decode_audio3(priv->codecContext, priv->m_tmpBuffer, &data_size, &priv->packet);
#endif

      if (ret < 0 || ret > priv->pkt_len) {
#if 0
        dbg(0, "audio decode error");
        return -1;
#endif
        priv->pkt_len = 0;
        ret = 0;
        continue;
      }

      priv->pkt_len -= ret; priv->pkt_ptr += ret;

      /* sample exact alignment  */
      if (priv->packet.pts != AV_NOPTS_VALUE) {
        priv->decoder_clock = priv->samplerate * av_q2d(priv->formatContext->streams[priv->audioStream]->time_base) * priv->packet.pts;
      } else {
        dbg(0, "!!! NO PTS timestamp in file");
        priv->decoder_clock += (data_size>>1) / priv->channels;
      }

      if (data_size > 0) {
        priv->m_tmpBufferLen+= (data_size>>1); // 2 bytes per sample
      }

      /* align buffer after seek. */
      if (priv->seek_frame > 0) {
        const int diff = priv->output_clock-priv->decoder_clock;
        if (diff < 0) { 
          /* seek ended up past the wanted sample */
          gwarn("audio seek failed.");
          return -1;
        } else if (priv->m_tmpBufferLen < (diff*priv->channels)) {
          /* wanted sample not in current buffer - keep going */
          dbg(2, " !!! seeked sample was not in decoded buffer. frames-to-go: %li", diff);
          priv->m_tmpBufferLen = 0;
        } else if (diff!=0 && data_size > 0) {
          /* wanted sample is in current buffer but not at the beginnning */
          dbg(2, " !!! sync buffer to seek. (diff:%i)", diff);
          priv->m_tmpBufferStart+= diff*priv->codecContext->channels;
          priv->m_tmpBufferLen  -= diff*priv->codecContext->channels;
#if 1
          memmove(priv->m_tmpBuffer, priv->m_tmpBufferStart, priv->m_tmpBufferLen);
          priv->m_tmpBufferStart = priv->m_tmpBuffer;
#endif
          priv->seek_frame = 0;
          priv->decoder_clock += diff;
        } else if (data_size > 0) {
          dbg(2, "Audio exact sync-seek (%"PRIi64" == %"PRIi64")", priv->decoder_clock, priv->seek_frame);
          priv->seek_frame = 0;
        } else {
          dbg(0, "Error: no audio data in packet");
        }
      }
      //dbg(0, "PTS: decoder:%"PRIi64". - want: %"PRIi64, priv->decoder_clock, priv->output_clock);
      //dbg(0, "CLK: frame:  %"PRIi64"  T:%.3fs",priv->decoder_clock, (float) priv->decoder_clock/priv->samplerate);
    }
  }
  if (written != frames) {
    dbg(2, "short-read");
  }
  return written * priv->channels;
}

int64_t ad_seek_ffmpeg(void *sf, int64_t pos) {
  ffmpeg_audio_decoder *priv = (ffmpeg_audio_decoder*) sf;
  if (!sf) return -1;
  if (pos == priv->output_clock) return pos;

  /* flush internal buffer */
  priv->m_tmpBufferLen = 0;
  priv->seek_frame = pos;
  priv->output_clock = pos;
  priv->pkt_len = 0; priv->pkt_ptr = NULL;
  priv->decoder_clock = 0;

#if 0
  /* TODO seek at least 1 packet before target.
   * for mpeg compressed files, the
   * output may depend on past frames! */
  if (pos > 8192) pos -= 8192;
  else pos = 0;
#endif

  const int64_t timestamp = pos / av_q2d(priv->formatContext->streams[priv->audioStream]->time_base) / priv->samplerate;
  dbg(2, "seek frame:%"PRIi64" - idx:%"PRIi64, pos, timestamp);
  
  av_seek_frame(priv->formatContext, priv->audioStream, timestamp, AVSEEK_FLAG_ANY | AVSEEK_FLAG_BACKWARD);
  avcodec_flush_buffers(priv->codecContext);
  return pos;
}

int ad_eval_ffmpeg(const char *f) { 
  char *ext = strrchr(f, '.');
  if (!ext) return 10;
  // libavformat.. guess_format.. 
  return 40;
}
#endif

const static ad_plugin ad_ffmpeg = {
#ifdef HAVE_FFMPEG
  &ad_eval_ffmpeg,
  &ad_open_ffmpeg,
  &ad_close_ffmpeg,
  &ad_info_ffmpeg,
  &ad_seek_ffmpeg,
  &ad_read_ffmpeg
#else
  &ad_eval_null,
  &ad_open_null,
  &ad_close_null,
  &ad_info_null,
  &ad_seek_null,
  &ad_read_null
#endif
};

/* dlopen handler */
const ad_plugin * get_ffmpeg() {
#ifdef HAVE_FFMPEG
  static int ffinit = 0;
  if (!ffinit) {
    ffinit=1;
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(53, 5, 0)
    avcodec_init();
#endif
    av_register_all();
    avcodec_register_all();
    av_log_set_level(_debug_ > 1 ? AV_LOG_VERBOSE : AV_LOG_QUIET);
  }
#endif
  return &ad_ffmpeg;
}
