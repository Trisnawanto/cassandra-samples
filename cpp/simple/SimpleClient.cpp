#include <iostream>

#include "SimpleClient.hpp"

namespace example {

using namespace std;

const std::string TAGS_COLUMN("tags");
const std::string TITLE_COLUMN("title");
const std::string ARTIST_COLUMN("artist");
const std::string ALBUM_COLUMN("album");

// auxiliary functions

inline CassError printError(CassError error) 
{
    cout << cass_error_desc(error) << "\n";
    return error;
}

inline CassError SimpleClient::executeStatement(const char* cqlStatement, const CassResult** results /* = NULL */)
{
    CassError rc = CASS_OK;
    CassFuture* result_future = NULL;
    
    cout << "Executing " << cqlStatement << "\n";
    
    CassString query = cass_string_init(cqlStatement);
    CassStatement* statement = cass_statement_new(query, 0);
    result_future = cass_session_execute(session, statement);
    cass_future_wait(result_future);
    
    rc = cass_future_error_code(result_future);
    if (rc == CASS_OK) 
    {
        cout << "Statement " << cqlStatement << " executed successully." << "\n";
        if ( results != NULL )
        {
            *results = cass_future_get_result(result_future);
        }
    } 
    else 
    {
        return printError(rc);
    }
    cass_statement_free(statement);
    cass_future_free(result_future);
    
    return rc;
}

CassError SimpleClient::connect(const string nodes) 
{
    CassError rc = CASS_OK;
    cout << "Connecting to " << nodes << "\n";
    cluster = cass_cluster_new();
    session = cass_session_new();
    CassFuture* connect_future = NULL;
    
    cass_cluster_set_contact_points(cluster, "127.0.0.1");

    connect_future = cass_session_connect(session, cluster);
    
    cass_future_wait(connect_future);
    rc = cass_future_error_code(connect_future);

    if ( rc == CASS_OK )
    {
        cout << "Connected." << "\n";
        CassSchema* schemaMetadata = cass_session_get_schema(sssion);
        CassIterator* iter = cass_iterator_from_schema(schemaMetadata);
        
        cass_iterator_free(iter);
        cass_schema_free(schemaMetadata);
    }
    else
    {
        return printError(rc);
    }
    cass_future_free(connect_future);
    return rc;
}

CassError SimpleClient::createSchema() 
{
    CassError rc = CASS_OK;
    
    cout << "Creating simplex keyspace." << endl;
    rc = executeStatement("CREATE KEYSPACE IF NOT EXISTS simplex WITH replication = {'class':'SimpleStrategy', 'replication_factor':3};");
    rc = executeStatement("CREATE TABLE IF NOT EXISTS simplex.songs (id uuid PRIMARY KEY,title text,album text,artist text,tags set<text>,data blob);");
    rc = executeStatement("CREATE TABLE IF NOT EXISTS simplex.playlists (id uuid,title text,album text,artist text,song_id uuid,PRIMARY KEY (id, title, album, artist));");
    
    return rc;
}

CassError SimpleClient::loadData() 
{
    CassError rc = CASS_OK;
    
    cout << "Loading data into simplex keyspace." << endl;
    
    rc = executeStatement("INSERT INTO simplex.songs (id, title, album, artist, tags) VALUES (756716f7-2e54-4715-9f00-91dcbea6cf50,'La Petite Tonkinoise','Bye Bye Blackbird','Joséphine Baker',{'jazz', '2013'});");
    rc = executeStatement("INSERT INTO simplex.songs (id, title, album, artist, tags) VALUES (f6071e72-48ec-4fcb-bf3e-379c8a696488,'Die Mösch','In Gold','Willi Ostermann',{'kölsch', '1996', 'birds'});");
    rc = executeStatement("INSERT INTO simplex.songs (id, title, album, artist, tags) VALUES (fbdf82ed-0063-4796-9c7c-a3d4f47b4b25,'Memo From Turner','Performance','Mick Jager',{'soundtrack', '1991'});");
    
    rc = executeStatement("INSERT INTO simplex.playlists (id, song_id, title, album, artist) VALUES (2cc9ccb7-6221-4ccb-8387-f22b6a1b354d,756716f7-2e54-4715-9f00-91dcbea6cf50,'La Petite Tonkinoise','Bye Bye Blackbird','Joséphine Baker');");
    rc = executeStatement("INSERT INTO simplex.playlists (id, song_id, title, album, artist) VALUES (2cc9ccb7-6221-4ccb-8387-f22b6a1b354d,f6071e72-48ec-4fcb-bf3e-379c8a696488,'Die Mösch','In Gold','Willi Ostermann');");
    rc = executeStatement("INSERT INTO simplex.playlists (id, song_id, title, album, artist) VALUES (3fd2bedf-a8c8-455a-a462-0cd3a4353c54,fbdf82ed-0063-4796-9c7c-a3d4f47b4b25,'Memo From Turner','Performance','Mick Jager');");
    
    return rc;
}

CassError SimpleClient::querySchema()
{
    CassError rc = CASS_OK;
    const CassResult* results;
    
    cout << "Querying the simplex.playlists table." << endl;
    
    rc = executeStatement("SELECT title, artist, album FROM simplex.playlists WHERE id = 2cc9ccb7-6221-4ccb-8387-f22b6a1b354d;", &results);
    
    CassIterator* rows = cass_iterator_from_result(results);
    
    if ( rc == CASS_OK ) 
    {
        while ( cass_iterator_next(rows) ) 
        {
            const CassRow* row = cass_iterator_get_row(rows);
            
            CassString title, artist, album;
            
            cass_value_get_string(cass_row_get_column(row, 0), &title);
            cass_value_get_string(cass_row_get_column(row, 1), &artist);
            cass_value_get_string(cass_row_get_column(row, 2), &album);
            
            cout << "title: " << title.data << ", artist: " << artist.data << ", album: " << album.data << "\n";
        }
        cass_result_free(results);
        cass_iterator_free(rows);
    }
    
    return rc;
}

CassError SimpleClient::updateSchema() 
{
    CassError rc = CASS_OK;
    const CassResult* results;
    
    cout << "Updating the simplex.songs table." << endl;
    rc = executeStatement("UPDATE simplex.songs SET tags = tags + { 'entre-deux-guerres' } WHERE id = 756716f7-2e54-4715-9f00-91dcbea6cf50;");
    
    if ( rc != CASS_OK )
    {
        return rc;
    }
    
    cout << "Querying the simplex.songs table." << endl;
    rc = executeStatement("SELECT title, artist, album, tags FROM simplex.songs WHERE id = 756716f7-2e54-4715-9f00-91dcbea6cf50;", &results);
    
    CassIterator* rows = cass_iterator_from_result(results);
    
    if ( rc == CASS_OK ) 
    {
        cout << TITLE_COLUMN << "\t" << ALBUM_COLUMN << "\t" << ARTIST_COLUMN << "\t" << TAGS_COLUMN << endl;
        while ( cass_iterator_next(rows) ) 
        {
            const CassRow* row = cass_iterator_get_row(rows);
            
            CassString title, artist, album;
            
            cass_value_get_string(cass_row_get_column(row, 0), &title);
            cass_value_get_string(cass_row_get_column(row, 1), &artist);
            cass_value_get_string(cass_row_get_column(row, 2), &album);
            cout << title.data << "\t" << artist.data << "\t" << album.data << "\t{ ";
            const CassValue* tags = cass_row_get_column(row, 3);
            CassIterator* tagsIter = cass_iterator_from_collection(tags);
            CassString tag;
            if ( cass_iterator_next(tagsIter) )
            {
                cass_value_get_string( cass_iterator_get_value(tagsIter), &tag );
                cout << tag.data;
            }
            while ( cass_iterator_next(tagsIter) )
            {
                cass_value_get_string( cass_iterator_get_value(tagsIter), &tag );
                cout << ", " << tag.data;
            }
            cout << " }\n";
            cass_iterator_free(tagsIter);
        }
        cass_result_free(results);
        cass_iterator_free(rows);
    }
    
    return rc;
}

CassError SimpleClient::dropSchema(const std::string keyspace) 
{
    CassError rc = CASS_OK;
    CassFuture* result_future = NULL;
    cout << "Dropping " + keyspace + "" << "\n";
    CassString query = cass_string_init("DROP KEYSPACE simplex;");
    CassStatement* statement = cass_statement_new(query, 0);
    result_future = cass_session_execute(session, statement);
    cass_future_wait(result_future);
    
    rc = cass_future_error_code(result_future);
    if (rc != CASS_OK) 
    {
        return printError(rc);
    }
    cass_statement_free(statement);
    cass_future_free(result_future);
    return rc;
}

void SimpleClient::close() 
{
    cout << "Closing down cluster connection." << "\n";
    cass_session_close(session);
    cass_cluster_free(cluster);
}

} // end namespace example




