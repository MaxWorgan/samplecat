#include "../../config.h"
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include "file_manager.h"
#include "typedefs.h"
#include "mimetype.h"
#include <gimp/gimpaction.h>
#include <gimp/gimpactiongroup.h>

#include "support.h"
#include "types.h"
#include "sample.h"
#include "db/sqlite.h"

static sqlite3* db;
sqlite3_stmt* ppStmt = NULL;
static SamplecatResult result;
extern int debug;
#define MAX_LEN 256 //temp!

enum {
	COLUMN_ID,
	COLUMN_FILENAME,
	COLUMN_DIR,
	COLUMN_KEYWORDS,
	COLUMN_PIXBUF,
	COLUMN_LENGTH,
	COLUMN_SAMPLERATE,
	COLUMN_CHANNELS,
	COLUMN_ONLINE,
	COLUMN_LASTCHECKED,
	COLUMN_MIMETYPE,
	COLUMN_NOTES,
	COLUMN_COLOUR,
};

static gboolean sqlite__update_int(int id, int, const char*);


void
sqlite__create_db()
{
}


int
sqlite__connect()
{
	PF;
	int rc;

	if(!ensure_config_dir()) return FALSE;
	char* db_name = g_strdup_printf("%s/.config/" PACKAGE "/" PACKAGE ".sqlite", g_get_home_dir());
	rc = sqlite3_open(db_name, &db); //if the file doesnt exist, it be created.
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		fprintf(stderr, "file=%s", db_name);
		sqlite3_close(db);
		return FALSE;
	}
	g_free(db_name);

	int on_create_table(void* NotUsed, int argc, char** argv, char** azColName)
	{
		int i;
		for (i=0; i<argc; i++) {
			printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		}
		printf("\n");
		return 0;
	}

	int table_exists = FALSE;
	if (!table_exists) {
		char* errMsg = 0;
		//sqlite doesnt appear to have a date type so we use INTERGER instead. check the size.
		//TODO check VARCHAR is appropriate for Notes column.
		int n = sqlite3_exec(db, "CREATE TABLE samples ("
			"id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
			"filename VARCHAR(100), filedir VARCHAR(100), keywords VARCHAR(60), pixbuf BLOB, length int(22), sample_rate int(11), channels int(4), "
			"online int(1), last_checked int(11), mimetype VARCHAR(32), notes VARCHAR(256), colour INTEGER(5))",
			on_create_table, 0, &errMsg);
		if (n != SQLITE_OK) {
			if (!strstr(errMsg, "already exists")) {
				perr("Sqlite error: %s\n", errMsg);
				return FALSE;
			}
		}
		else return TRUE;
	}
	return TRUE;
}


void
sqlite__disconnect()
{
	sqlite3_close(db);
}


int
sqlite__insert(sample* sample, MIME_type *mime_type)
{
	//TODO is supposed to return the new id?

	int on_insert(void* NotUsed, int argc, char** argv, char** azColName)
	{
		PF;
		return 0;
	}

	int ret = 0;

	gchar* filedir = g_path_get_dirname(sample->filename);
	gchar* filename = g_path_get_basename(sample->filename);
	gchar* mime_str = g_strdup_printf("%s/%s", mime_type->media_type, mime_type->subtype);
	int colour = 0;
	gchar* sql = g_strdup_printf(
		"INSERT INTO samples(filename,filedir,length,sample_rate,channels,online,mimetype,colour) "
		"VALUES ('%s','%s','%i','%i','%i','%i','%s', '%i')",
		filename, filedir, sample->length, sample->sample_rate, sample->channels, 1, mime_str, colour
	);
	dbg(1, "sql=%s", sql);

	char* errMsg = 0;
	int n = sqlite3_exec(db, sql, on_insert, 0, &errMsg);
	if (n != SQLITE_OK) {
		gwarn("sqlite error: %s", errMsg);
		ret = -1;
	}

	sqlite3_int64 idx = sqlite3_last_insert_rowid(db);

	dbg(1, "n_changes=%i", sqlite3_changes(db));

	g_free(sql);
	g_free(filedir);
	g_free(filename);
	g_free(mime_str);
	return (int)idx;
}


gboolean
sqlite__delete_row(int id)
{
	int on_insert(void* NotUsed, int argc, char** argv, char** azColName)
	{
		PF;
		return 0;
	}

	gboolean ok = true;
	gchar* sql = g_strdup_printf("DELETE FROM samples WHERE id=%i", id);
	dbg(1, "sql=%s", sql);
	char* errMsg = 0;
	int n;
	if((n = sqlite3_exec(db, sql, on_insert, 0, &errMsg)) != SQLITE_OK){
		perr("delete failed! sql=%s\n", errMsg);
		ok = false;
	}
	g_free(sql);
	return ok;
}


static gboolean
sqlite__update_int(int id, int value, const char* field)
{
	g_return_val_if_fail(id, false);

	int on_update(void* NotUsed, int argc, char** argv, char** azColName)
	{
		PF;
		return 0;
	}

	gboolean ok = true;
	gchar* sql = g_strdup_printf("UPDATE samples SET %s=%i WHERE id=%i", field, value, id);
	dbg(1, "sql=%s", sql);
	char* errMsg = 0;
    int n = sqlite3_exec(db, sql, on_update, 0, &errMsg);
    if (n != SQLITE_OK) {
		gwarn("sqlite error: %s", errMsg);
		ok = false;
	}
	g_free(sql);
	return ok;
}


gboolean
sqlite__update_colour(int id, int colour)
{
	return sqlite__update_int(id, colour, "colour");
}


gboolean
sqlite__update_keywords(int id, const char* keywords)
{
	int on_update(void* NotUsed, int argc, char** argv, char** azColName)
	{
		PF;
		return 0;
	}

	g_return_val_if_fail(id, false);
	gboolean ok = true;
	gchar* sql = g_strdup_printf("UPDATE samples SET keywords='%s' WHERE id=%u", keywords, id);
	dbg(1, "sql=%s", sql);
	char* errMsg = 0;
    int n = sqlite3_exec(db, sql, on_update, 0, &errMsg);
    if (n != SQLITE_OK) {
		gwarn("sqlite error: %s", errMsg);
		ok = false;
	}
	g_free(sql);
	return ok;
}


gboolean
sqlite__update_notes(int id, const char* notes)
{
	int on_update(void* NotUsed, int argc, char** argv, char** azColName)
	{
		PF;
		return 0;
	}

	g_return_val_if_fail(id, false);
	gboolean ok = true;
	gchar* sql = g_strdup_printf("UPDATE samples SET notes='%s' WHERE id=%u", notes, id);
	dbg(1, "sql=%s", sql);
	char* errMsg = 0;
    int n = sqlite3_exec(db, sql, on_update, 0, &errMsg);
    if (n != SQLITE_OK) {
		gwarn("sqlite error: %s", errMsg);
		ok = false;
	}
	g_free(sql);
	return ok;
}


gboolean
sqlite__update_pixbuf(sample* sample)
{
	GdkPixbuf* pixbuf = sample->pixbuf;
	g_return_val_if_fail(pixbuf, false);

	gboolean ok = true;
	sqlite3_stmt* ppStmt = NULL;
	gchar* sql = g_strdup_printf("UPDATE samples SET pixbuf=? WHERE id=%u", sample->id);
	int rc = sqlite3_prepare_v2(db, sql, -1, &ppStmt, 0);
	if (rc == SQLITE_OK && ppStmt) {
		GdkPixdata pixdata;
		gdk_pixdata_from_pixbuf(&pixdata, pixbuf, 0);
		guint length;
		guint8* buf = gdk_pixdata_serialize(&pixdata, &length); //this is free'd by the sqlite3_bind_blob callback

		sqlite3_bind_blob(ppStmt, 1, buf, length, g_free);
		while ((rc = sqlite3_step(ppStmt)) == SQLITE_ROW) {
		/*
			int i; for (i = 0; i < sqlite3_column_count(ppStmt); ++i){
				//print_col(ppStmt, i);
				dbg(0, "  column.");
			}
			printf("\n");
		*/
		}
		if(rc != SQLITE_DONE) pwarn("step: code=%i error=%s\n", rc, sqlite3_errmsg(db));
		if((rc = sqlite3_finalize(ppStmt)) != SQLITE_OK){
			pwarn("finalize error: %s", sqlite3_errmsg(db));
		}
		dbg(1, "update done");
	}
	else pwarn("prepare not ok! code=%i %s\n", rc, sqlite3_errmsg(db));
	if(sqlite3_changes(db) != 1){ pwarn("n_changes=%i\n", sqlite3_changes(db)); ok = false; }
	g_free(sql);

	return ok;
}


gboolean
sqlite__update_online(int id, gboolean online)
{
	g_return_val_if_fail(id, false);

	int on_update(void* NotUsed, int argc, char** argv, char** azColName)
	{
		PF;
		return 0;
	}

	gboolean ok = true;
	gchar* sql = g_strdup_printf("UPDATE samples SET online=%i, last_checked=datetime('now') WHERE id=%i", online, id);
	dbg(1, "sql=%s", sql);
	char* errMsg = 0;
    int n = sqlite3_exec(db, sql, on_update, 0, &errMsg);
    if (n != SQLITE_OK) {
		gwarn("sqlite error: %s", errMsg);
		ok = false;
	}
	g_free(sql);
	return ok;
}


#ifdef NEVER
static int
delete_row (int argc, char *argv[])
{
	int on_delete(void* NotUsed, int argc, char** argv, char** azColName)
	{
		PF;
		return 0;
	}

	int n = 0, ret = 0;
	char* errMsg = 0;

	/* Allocate memory */
	char* domain = g_malloc (MAX_LEN);
	char* query = g_malloc (MAX_LEN);

	/* Request user input */
	printf ("Enter domain, e.g. mail.yahoo.com: ");
	fgets (domain, MAX_LEN, stdin);

	/* remove trailing newline */
	domain[strlen(domain) - 1] = '\0';

	/* Execute to database */
	sqlite3_snprintf (MAX_LEN, query, "DELETE FROM mypass WHERE domain='%q'", domain);
	printf ("query = %s\n", query);

	/* Check return result */
	n = sqlite3_exec(db, query, on_delete, 0, &errMsg);
	if (n != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errMsg);
		ret = 1;
	}
	else
		ret = 0;

	/* Free allocated memory */
	g_free(domain);
	g_free(query);
	return ret;
}
#endif


gboolean
sqlite__search_iter_new(char* search, char* dir, const char* category, int* n_results)
{
	PF;
	static int count = 0;

	void select_count(const char* where)
	{
		int select_callback(void* NotUsed, int argc, char** argv, char** azColName){ count = atoi(argv[0]); return 0; }

		char* sql = g_strdup_printf("SELECT COUNT(*) FROM samples WHERE 1 %s", where);
		char* errMsg = NULL;
		sqlite3_exec(db, sql, select_callback, 0, &errMsg);
	}

	gboolean ok = true;

	char* where = g_strdup_printf("%s", "");
	if(search && strlen(search)){
		char* where2 = g_strdup_printf("%s AND (filename LIKE '%%%s%%' OR filedir LIKE '%%%s%%' OR keywords LIKE '%%%s%%') ", where, search, search, search);
		g_free(where);
		where = where2;
	}
	if(category){
		gchar* where2 = g_strdup_printf("%s AND keywords LIKE '%%%s%%' ", where, category);
		g_free(where);
		where = where2;
	}
	if(dir && strlen(dir)){
		gchar* where2 = g_strdup_printf("%s AND filedir='%s' ", where, dir);
		g_free(where);
		where = where2;
	}

	char* sql = g_strdup_printf("SELECT * FROM samples WHERE 1 %s", where);
	dbg(1, "sql=%s", sql);

	if(ppStmt) gwarn("ppStmt not reset from previous query?");
	int n = sqlite3_prepare_v2(db, sql, -1, &ppStmt, NULL);
	if(n == SQLITE_OK && ppStmt){
		count = 0;
		if(n_results){
			select_count(where);
			*n_results = count;
		}
	}
	else{
		gwarn("failed to create prepared statement. sql=%s", sql);
		ok = false;
	}

#if 0
	int select_callback(void* NotUsed, int argc, char** argv, char** azColName)
	{
		//this is a per-row callback. If there are no selected rows, it will not be called.

		dbg(0, "n_columns=%i", argc);
		int i; for(i=0; i<argc; i++) printf("  %s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		printf ("\n");

		return 0; //non zero values cause SQLITE_ABORT to occur.
	}

	char* errMsg = NULL;
	n = sqlite3_exec(db, sql, select_callback, 0, &errMsg);
	g_free(sql);
	if(n != SQLITE_OK){
		gwarn("sqlite error: %i: %s", n, errMsg);
		sqlite3_free(errMsg);
		return FALSE;
	}
#endif
	g_free(sql);
	g_free(where);
	return ok;
}


SamplecatResult*
sqlite__search_iter_next(unsigned long** lengths)
{
	memset(&result, 0, sizeof(SamplecatResult));

	int n = sqlite3_step(ppStmt);
	if(n == SQLITE_ROW){

		//deserialise the pixbuf field:
		GdkPixdata pixdata;
		GdkPixbuf* pixbuf = NULL;
		const char* blob = sqlite3_column_blob(ppStmt, COLUMN_PIXBUF);
		if(blob){
			int length = sqlite3_column_bytes(ppStmt, COLUMN_PIXBUF);
			dbg(2, "pixbuf_length=%i", length);
			if(gdk_pixdata_deserialize(&pixdata, length, (guint8*)blob, NULL)){
				pixbuf = gdk_pixbuf_from_pixdata(&pixdata, TRUE, NULL);
			}
		}

		result.idx         = sqlite3_column_int(ppStmt, COLUMN_ID);
		result.sample_name = (char*)sqlite3_column_text(ppStmt, COLUMN_FILENAME);
		result.dir         = (char*)sqlite3_column_text(ppStmt, COLUMN_DIR);
		result.keywords    = (char*)sqlite3_column_text(ppStmt, COLUMN_KEYWORDS);
		result.length      = sqlite3_column_int(ppStmt, COLUMN_LENGTH);
		result.sample_rate = sqlite3_column_int(ppStmt, COLUMN_SAMPLERATE);
		result.channels    = sqlite3_column_int(ppStmt, COLUMN_CHANNELS);
		result.overview    = (GdkPixbuf*)sqlite3_column_blob(ppStmt, COLUMN_PIXBUF);
		result.notes       = (char*)sqlite3_column_text(ppStmt, COLUMN_NOTES);
		result.colour      = sqlite3_column_int(ppStmt, COLUMN_COLOUR);
		result.mimetype    = (char*)sqlite3_column_text(ppStmt, COLUMN_MIMETYPE);
		result.online      = sqlite3_column_int(ppStmt, COLUMN_ONLINE);
		result.overview    = pixbuf;

		//sqlite3_result_blob(sqlite3_context*, const void*, int, void(*)(void*));

		return &result;
	}
	else return NULL;
}


void
sqlite__search_iter_free()
{
	sqlite3_finalize(ppStmt);
	ppStmt = NULL;
}


void
sqlite__dir_iter_new()
{
	#define SQLITE_DIR_LIST_QRY "SELECT DISTINCT filedir FROM samples ORDER BY filedir"

	if(ppStmt) gwarn("ppStmt not reset from previous query?");
	int n = sqlite3_prepare_v2(db, SQLITE_DIR_LIST_QRY, -1, &ppStmt, NULL);
	if(ppStmt && n == SQLITE_OK){
		/*
		count = 0;
		if(n_results){
			select_count(where);
			*n_results = count;
		}
		*/
	}
	else{
		gwarn("failed to create prepared statement. sql=%s", SQLITE_DIR_LIST_QRY);
	}
}


char*
sqlite__dir_iter_next()
{
	int n = sqlite3_step(ppStmt);
	if(n == SQLITE_ROW){
		return (char*)sqlite3_column_text(ppStmt, 0);
	}
	else return NULL;
}


void
sqlite__dir_iter_free()
{
	sqlite3_finalize(ppStmt);
	ppStmt = NULL;
}


