#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "forestdb/include/libforestdb/forestdb.h"

#include "test_common.h"

void append_result(std::string& str_out,
                   const std::string& stat_name,
                   uint64_t num,
                   uint64_t elapsed) {
    char buffer[256];
    sprintf(buffer, "%-16s: %10.1f ops/sec\n",
            stat_name.c_str(), (double)num * 1000000 / elapsed);
    str_out += buffer;
}

int basic_example(uint64_t num) {
    fdb_status fs;
    fdb_file_handle* db_file;
    fdb_kvs_handle* db;

    fdb_config f_config = fdb_get_default_config();
    // Set buffer cache size: 1 GB.
    f_config.buffercache_size = (uint64_t)1024*1024*1024;

    fdb_kvs_config kvs_config = fdb_get_default_kvs_config();

    std::string db_path = "./tmp_db";
    TestSuite::clearTestFile(db_path);

    // Open ForestDB file handle.
    // Each file handle can have multiple KV stores (indexes).
    fs = fdb_open(&db_file, db_path.c_str(), &f_config);
    CHK_OK(fs == FDB_RESULT_SUCCESS);

    // Open default KV store handle.
    fs = fdb_kvs_open_default(db_file, &db, &kvs_config);
    CHK_OK(fs == FDB_RESULT_SUCCESS);

    char keybuf[32];
    char valuebuf[32];
    TestSuite::Timer timer(0);
    uint64_t elapsed;
    std::string result_str = "\n";

    // Bulk load: insert k-v pairs without updating B+tree.
    timer.reset();
    for (uint64_t ii=0; ii<num; ++ii) {
        sprintf(keybuf, "k%07zu", ii);
        sprintf(valuebuf, "v%07zu", ii);
        fs = fdb_set_kv(db, keybuf, 8, valuebuf, 8);
        CHK_OK(fs == FDB_RESULT_SUCCESS);
    }
    // Update B+tree at once at the end, and then call fsync().
    fs = fdb_commit(db_file, FDB_COMMIT_NORMAL);
    CHK_OK(fs == FDB_RESULT_SUCCESS);

    elapsed = timer.getTimeUs();
    append_result(result_str, "bulk load", num, elapsed);

    // Get: randomly retrieve k-v pairs.
    timer.reset();
    for (uint64_t ii=0; ii<num; ++ii) {
        size_t r = std::rand() % num;
        sprintf(keybuf, "k%07zu", r);

        void* value_out;
        size_t valuelen_out;
        fs = fdb_get_kv(db, keybuf, 8, &value_out, &valuelen_out);
        CHK_OK(fs == FDB_RESULT_SUCCESS);

        // Should free the returned value.
        free(value_out);
    }
    elapsed = timer.getTimeUs();
    append_result(result_str, "get", num, elapsed);

    // Set (update): randomly update k-v pairs (write batch size: 1000).
    timer.reset();
    for (uint64_t ii=0; ii<num; ++ii) {
        size_t r = std::rand() % num;
        sprintf(keybuf, "k%07zu", r);
        sprintf(valuebuf, "v%07zu", r);

        fs = fdb_set_kv(db, keybuf, 8, valuebuf, 8);
        CHK_OK(fs == FDB_RESULT_SUCCESS);

        if (ii && ii % 1000 == 0) {
            // For every 1000 set operation,
            // update B+tree and then call fsync().
            // Note that fdb_commit() is very expensive operation so that
            // should avoid calling it for every set() operation.
            fs = fdb_commit(db_file, FDB_COMMIT_NORMAL);
            CHK_OK(fs == FDB_RESULT_SUCCESS);
        }
    }
    elapsed = timer.getTimeUs();
    append_result(result_str, "set", num, elapsed);

    // Delete all k-v paris.
    timer.reset();
    for (uint64_t ii=0; ii<num; ++ii) {
        sprintf(keybuf, "k%07zu", ii);
        fs = fdb_del_kv(db, keybuf, 8);
        CHK_OK(fs == FDB_RESULT_SUCCESS);
    }
    // Update B+tree at the end, and then call fsync().
    fs = fdb_commit(db_file, FDB_COMMIT_NORMAL);
    CHK_OK(fs == FDB_RESULT_SUCCESS);

    elapsed = timer.getTimeUs();
    append_result(result_str, "bulk delete", num, elapsed);

    // Close KV store handle.
    fs = fdb_kvs_close(db);
    CHK_OK(fs == FDB_RESULT_SUCCESS);

    // Close DB file handle.
    fs = fdb_close(db_file);
    CHK_OK(fs == FDB_RESULT_SUCCESS);

    TestSuite::setResultMessage(result_str);

    return 0;
}

int main(int argc, char** argv) {
    TestSuite ts(argc, argv);

    ts.doTest( "basic example",
               basic_example,
               TestRange<uint64_t>(
                   (std::vector<uint64_t>){1000, 10000, 100000} ) );

    return 0;
}

