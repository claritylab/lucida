#include <mongoc.h>

#include "mongoc-client-private.h"
#include "mongoc-uri-private.h"

#include "mock_server/mock-server.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "mongoc-tests.h"
#include "TestSuite.h"
#include "test-libmongoc.h"
#include "test-conveniences.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "cluster-test"


static uint32_t server_id_for_reads (mongoc_cluster_t *cluster)
{
   bson_error_t error;
   mongoc_server_stream_t *server_stream;
   uint32_t id;

   server_stream = mongoc_cluster_stream_for_reads (cluster, NULL, &error);
   ASSERT_OR_PRINT (server_stream, error);
   id = server_stream->sd->id;

   mongoc_server_stream_cleanup (server_stream);

   return id;
}


static void
test_get_max_bson_obj_size (void)
{
   mongoc_server_description_t *sd;
   mongoc_cluster_node_t *node;
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   int32_t max_bson_obj_size = 16;
   uint32_t id;

   /* single-threaded */
   client = test_framework_client_new ();
   assert (client);

   id = server_id_for_reads (&client->cluster);
   sd = (mongoc_server_description_t *)mongoc_set_get (client->topology->description.servers, id);
   sd->max_bson_obj_size = max_bson_obj_size;
   assert (max_bson_obj_size == mongoc_cluster_get_max_bson_obj_size (&client->cluster));

   mongoc_client_destroy (client);

   /* multi-threaded */
   pool = test_framework_client_pool_new ();
   client = mongoc_client_pool_pop (pool);

   id = server_id_for_reads (&client->cluster);
   node = (mongoc_cluster_node_t *)mongoc_set_get (client->cluster.nodes, id);
   node->max_bson_obj_size = max_bson_obj_size;
   assert (max_bson_obj_size == mongoc_cluster_get_max_bson_obj_size (&client->cluster));

   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
}

static void
test_get_max_msg_size (void)
{
   mongoc_server_description_t *sd;
   mongoc_cluster_node_t *node;
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   int32_t max_msg_size = 32;
   uint32_t id;

   /* single-threaded */
   client = test_framework_client_new ();
   id = server_id_for_reads (&client->cluster);

   sd = (mongoc_server_description_t *)mongoc_set_get (client->topology->description.servers, id);
   sd->max_msg_size = max_msg_size;
   assert (max_msg_size == mongoc_cluster_get_max_msg_size (&client->cluster));

   mongoc_client_destroy (client);

   /* multi-threaded */
   pool = test_framework_client_pool_new ();
   client = mongoc_client_pool_pop (pool);

   id = server_id_for_reads (&client->cluster);
   node = (mongoc_cluster_node_t *)mongoc_set_get (client->cluster.nodes, id);
   node->max_msg_size = max_msg_size;
   assert (max_msg_size == mongoc_cluster_get_max_msg_size (&client->cluster));

   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
}


#define ASSERT_CURSOR_ERR() do { \
      char *error_message = bson_strdup_printf ( \
         "Failed to read 4 bytes from socket within %d milliseconds.", \
         socket_timeout_ms); \
      assert (!future_get_bool (future)); \
      assert (mongoc_cursor_error (cursor, &error)); \
      ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_STREAM); \
      ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_STREAM_SOCKET); \
      ASSERT_CMPSTR (error.message, error_message); \
      bson_free (error_message); \
   } while (0)


#define START_QUERY(client_port_variable) do { \
      cursor = mongoc_collection_find (collection, \
                                       MONGOC_QUERY_NONE, \
                                       0, 0, 0, tmp_bson ("{}"), \
                                       NULL, NULL); \
      future = future_cursor_next (cursor, &doc); \
      request = mock_server_receives_query (server, "test.test", \
                                            MONGOC_QUERY_SLAVE_OK, 0, 0, \
                                            "{}", NULL); \
      client_port_variable = request_get_client_port (request); \
   } while (0)


#define CLEANUP_QUERY() do { \
      request_destroy (request); \
      future_destroy (future); \
      mongoc_cursor_destroy (cursor); \
   } while (0)


/* test that we reconnect a cluster node after disconnect */
static void
_test_cluster_node_disconnect (bool pooled)
{
   mock_server_t *server;
   const int32_t socket_timeout_ms = 100;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   const bson_t *doc;
   mongoc_cursor_t *cursor;
   future_t *future;
   request_t *request;
   uint16_t client_port_0, client_port_1;
   bson_error_t error;

   server = mock_server_with_autoismaster (0);
   mock_server_run (server);

   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, "socketTimeoutMS", socket_timeout_ms);

   if (pooled) {
      pool = mongoc_client_pool_new (uri);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (uri);
   }

   collection = mongoc_client_get_collection (client, "test", "test");

   /* query 0 fails. set client_port_0 to the port used by the query. */
   START_QUERY (client_port_0);
   if (pooled) {
      suppress_one_message ();
   }

   mock_server_resets (request);
   ASSERT_CURSOR_ERR ();
   CLEANUP_QUERY ();

   /* query 1 opens a new socket. set client_port_1 to the new port. */
   START_QUERY (client_port_1);
   ASSERT_CMPINT (client_port_1, !=, client_port_0);
   mock_server_replies_simple (request, "{'a': 1}");

   /* success! */
   BSON_ASSERT (future_get_bool (future));

   CLEANUP_QUERY ();
   mongoc_collection_destroy (collection);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
}


static void
test_cluster_node_disconnect_single (void)
{
   _test_cluster_node_disconnect (false);
}


static void
test_cluster_node_disconnect_pooled (void)
{
   _test_cluster_node_disconnect (true);
}


void
test_cluster_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/Cluster/test_get_max_bson_obj_size", test_get_max_bson_obj_size);
   TestSuite_Add (suite, "/Cluster/test_get_max_msg_size", test_get_max_msg_size);
   TestSuite_Add (suite, "/Cluster/disconnect/single", test_cluster_node_disconnect_single);
   TestSuite_Add (suite, "/Cluster/disconnect/pooled", test_cluster_node_disconnect_pooled);
}
