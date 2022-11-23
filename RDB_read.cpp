// ExampleConsole.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "RDBHandler.hh"
#include <fstream>
#include <string>
#include "vtd/TrafficParticipants.hpp"

#define DEFAULT_PORT        48190   /* for image port it should be 48192 */
#define DEFAULT_BUFFER      204800

char  szServer[128];             // Server to connect to
int   iPort     = DEFAULT_PORT;  // Port on server to connect to
std::string file_path = "output/data.csv";   //存文件的路径
std::ofstream file;
Vehicle ego_info;
std::vector<double> surVeh_info;
int surVeh_num = 1;


// function prototypes
void parseRDBMessage( RDB_MSG_t* msg, bool & isImage );
void parseRDBMessageEntry( const double & simTime, const unsigned int & simFrame, RDB_MSG_ENTRY_HDR_t* entryHdr );
void handleRDBitem( const double & simTime, const unsigned int & simFrame, RDB_OBJECT_STATE_t & item, bool isExtended );
void handleRDBitem( const double & simTime, const unsigned int & simFrame, RDB_ROAD_POS_t & item );
// void handleRDBitem( const double & simTime, const unsigned int & simFrame, RDB_IMAGE_t & item );
void sendTrigger( int & sendSocket, const double & simTime, const unsigned int & simFrame );


//
// Function: usage:
//
// Description:
//    Print usage information and exit
//
void usage()
{
    printf("usage: client [-p:x] [-s:IP]\n\n");
    printf("       -p:x      Remote port to send to\n");
    printf("       -s:IP     Server's IP address or hostname\n");
    exit(1);
}

//
// Function: ValidateArgs
//
// Description:
//    Parse the command line arguments, and set some global flags
//    to indicate what actions to perform
//
void ValidateArgs(int argc, char **argv)
{
    int i;
    
    strcpy( szServer, "127.0.0.1" );
 
    for(i = 1; i < argc; i++)
    {
        if ((argv[i][0] == '-') || (argv[i][0] == '/'))
        {
            switch (tolower(argv[i][1]))
            {
                case 'p':        // Remote port
                    if (strlen(argv[i]) > 3)
                        iPort = atoi(&argv[i][3]);
                    break;
                case 's':       // Server
                    if (strlen(argv[i]) > 3)
                        strcpy(szServer, &argv[i][3]);
                    break;
                default:
                    usage();
                    break;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    int           sClient;
    char* szBuffer = new char[DEFAULT_BUFFER];  // allocate on heap
    int           ret;

    struct sockaddr_in server;
    struct hostent    *host = NULL;

    static bool sSendData    = false;
    static bool sVerbose     = true;
    static bool sSendTrigger = false;

    // Parse the command line
    //
    ValidateArgs(argc, argv);
    
    //
    // Create the socket, and attempt to connect to the server
    //
    sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if ( sClient == -1 )
    {
        fprintf( stderr, "socket() failed: %s\n", strerror( errno ) );
        return 1;
    }
    
    int opt = 1;
    setsockopt ( sClient, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof( opt ) );

    server.sin_family      = AF_INET;
    server.sin_port        = htons(iPort);
    server.sin_addr.s_addr = inet_addr(szServer);
    
    //
    // If the supplied server address wasn't in the form
    // "aaa.bbb.ccc.ddd" it's a hostname, so try to resolve it
    //
    if ( server.sin_addr.s_addr == INADDR_NONE )
    {
        host = gethostbyname(szServer);
        if ( host == NULL )
        {
            fprintf( stderr, "Unable to resolve server: %s\n", szServer );
            return 1;
        }
        memcpy( &server.sin_addr, host->h_addr_list[0], host->h_length );
    }
	// wait for connection
	bool bConnected = false;

    while ( !bConnected )
    {
        if (connect( sClient, (struct sockaddr *)&server, sizeof( server ) ) == -1 )
        {
            fprintf( stderr, "connect() failed: %s\n", strerror( errno ) );
            sleep( 1 );
        }
        else
            bConnected = true;
    }

    fprintf( stderr, "connected!\n" );
    
    unsigned int  bytesInBuffer = 0;
    size_t        bufferSize    = sizeof( RDB_MSG_HDR_t );
    unsigned int  count         = 0;
    unsigned char *pData        = ( unsigned char* ) calloc( 1, bufferSize );

    // Send and receive data - forever!
    //
    
    file.open(file_path, std::ios::trunc); 
    file << "frame" << "," << "ego_.x_" << "," << "ego_.y_" << "," << "ego_.length_" << "," << "ego_.width_" << "," <<
        "ego_.speed_" << "," << "ego_.speed_x_" << "," << "ego_.speed_y_" << "," << "ego_.acc_" << "," <<
        "ego_.init_q_" << "," << "ego_.lane_posi_" << "," << "ego_.absolute_theta_" << "," << "ego_.relative_theta_"  ;
    for(int i = 0; i < surVeh_num; i++)
    {
        file << "," <<"sur_x" << "," << "sur_y" << "," << "sur_len" << "," << "sur_wid" << "," << "sur_vx" << "," << "sur_vy" << "," <<
        "sur_ax" << "," << "sur_ay" << "," << "sur_heading" ;
    }   
    file << std::endl;    
    file.close();
    for(;;)
    {
        bool bMsgComplete = false;

        // read a complete message
        while ( !bMsgComplete )
        {
            if ( sSendTrigger && !( count++ % 1000 ) )
              sendTrigger( sClient, 0, 0 );

            ret = recv( sClient, szBuffer, DEFAULT_BUFFER, 0 );

            if ( ret == -1 )
            {
                printf( "recv() failed: %s\n", strerror( errno ) );
                break;
            }

            if ( ret != 0 )
            {
                // do we have to grow the buffer??
                if ( ( bytesInBuffer + ret ) > bufferSize )
                {
                    pData      = ( unsigned char* ) realloc( pData, bytesInBuffer + ret );
                    bufferSize = bytesInBuffer + ret;
                }

                memcpy( pData + bytesInBuffer, szBuffer, ret );
                bytesInBuffer += ret;

                // already complete messagae?
                if ( bytesInBuffer >= sizeof( RDB_MSG_HDR_t ) )
                {
                    RDB_MSG_HDR_t* hdr = ( RDB_MSG_HDR_t* ) pData;

                    // is this message containing the valid magic number?
                    if ( hdr->magicNo != RDB_MAGIC_NO )
                    {
                        printf( "message receiving is out of sync; discarding data" );
                        bytesInBuffer = 0;
                    }

                    while ( bytesInBuffer >= ( hdr->headerSize + hdr->dataSize ) )
                    {
                        unsigned int msgSize = hdr->headerSize + hdr->dataSize;
                        bool         isImage = false;
                        
                        // print the message
                        if ( sVerbose )
                            Framework::RDBHandler::printMessage( ( RDB_MSG_t* ) pData, true );
                        
                        // now parse the message    
                        parseRDBMessage( ( RDB_MSG_t* ) pData, isImage );

                        // trigger after image has been handled
                        if ( sSendTrigger )
                            sendTrigger( sClient, ( ( RDB_MSG_t* ) pData )->hdr.simTime, ( ( RDB_MSG_t* ) pData )->hdr.frameNo );
                        
                        // this routine shall be implemented by a third party if applicable                    
                        //handleMsgByThirdParty( pData );

                        // remove message from queue
                        memmove( pData, pData + msgSize, bytesInBuffer - msgSize );
                        bytesInBuffer -= msgSize;

                        bMsgComplete = true;
                    }
                }
            }
        }
		// do some other stuff before returning to network reading
    }
    ::close(sClient);

    return 0;
}

void parseRDBMessage( RDB_MSG_t* msg, bool & isImage )
{
    if ( !msg )
      return;

    if ( !msg->hdr.dataSize )
        return;
    
    RDB_MSG_ENTRY_HDR_t* entry = ( RDB_MSG_ENTRY_HDR_t* ) ( ( ( char* ) msg ) + msg->hdr.headerSize );
    uint32_t remainingBytes    = msg->hdr.dataSize;
        
    while ( remainingBytes )
    {
        parseRDBMessageEntry( msg->hdr.simTime, msg->hdr.frameNo, entry );

        isImage |= ( entry->pkgId == RDB_PKG_ID_IMAGE );

        remainingBytes -= ( entry->headerSize + entry->dataSize );
        
        if ( remainingBytes )
          entry = ( RDB_MSG_ENTRY_HDR_t* ) ( ( ( ( char* ) entry ) + entry->headerSize + entry->dataSize ) );
    }
}

void parseRDBMessageEntry( const double & simTime, const unsigned int & simFrame, RDB_MSG_ENTRY_HDR_t* entryHdr )
{
    if ( !entryHdr )
        return;
    
    int noElements = entryHdr->elementSize ? ( entryHdr->dataSize / entryHdr->elementSize ) : 0;
    
    if ( !noElements )  // some elements require special treatment
    {
        switch ( entryHdr->pkgId )
        {
            case RDB_PKG_ID_START_OF_FRAME:
            {
                // fprintf( stderr, "void parseRDBMessageEntry: got start of frame\n" );
                surVeh_info.clear();
                ego_info.lane_posi_ = 4;
                break;
            }
            
            case RDB_PKG_ID_END_OF_FRAME:
            {
                // fprintf( stderr, "void parseRDBMessageEntry: got end of frame\n" );
                file.open(file_path, std::ios::app);
                file << simFrame << "," << ego_info.x_ << "," << ego_info.y_ << "," << ego_info.length_ << "," << ego_info.width_ << "," <<
                    ego_info.speed_ << "," << ego_info.speed_x_ << "," << ego_info.speed_y_ << "," << ego_info.acc_ << "," <<
                    ego_info.init_q_ << "," << ego_info.lane_posi_ << "," << ego_info.absolute_theta_ << "," << ego_info.relative_theta_;
                for(int i = 0; i < surVeh_info.size(); i++)
                {
                    file << "," << surVeh_info[i];
                }
                file << std::endl;
                file.close();
                break;
            }
                
                
            default:
                return;
                break;
        }
        return;
    }

    unsigned char ident   = 6;
    char*         dataPtr = ( char* ) entryHdr;
        
    dataPtr += entryHdr->headerSize;
        
    while ( noElements-- )
    {
        bool printedMsg = true;
            
        switch ( entryHdr->pkgId )
        {
/*
            case RDB_PKG_ID_COORD_SYSTEM:
                print( *( ( RDB_COORD_SYSTEM_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_COORD:
                print( *( ( RDB_COORD_t* ) dataPtr ), ident );
                break;
*/                    
            case RDB_PKG_ID_ROAD_POS:
                handleRDBitem( simTime, simFrame, *( ( RDB_ROAD_POS_t* ) dataPtr ) );
                // print( *( ( RDB_ROAD_POS_t* ) dataPtr ), ident );
                break;
/*                    
            case RDB_PKG_ID_LANE_INFO:
                print( *( ( RDB_LANE_INFO_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_ROADMARK:
                print( *( ( RDB_ROADMARK_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_OBJECT_CFG:
                print( *( ( RDB_OBJECT_CFG_t* ) dataPtr ), ident );
                break;
*/                    
            case RDB_PKG_ID_OBJECT_STATE:
                handleRDBitem( simTime, simFrame, *( ( RDB_OBJECT_STATE_t* ) dataPtr ), entryHdr->flags & RDB_PKG_FLAG_EXTENDED );
                break;
                    
/*            case RDB_PKG_ID_VEHICLE_SYSTEMS:
                print( *( ( RDB_VEHICLE_SYSTEMS_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_VEHICLE_SETUP:
                print( *( ( RDB_VEHICLE_SETUP_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_ENGINE:
                print( *( ( RDB_ENGINE_t* ) dataPtr ), entryHdr->flags & RDB_PKG_FLAG_EXTENDED, ident );
                break;
                    
            case RDB_PKG_ID_DRIVETRAIN:
                print( *( ( RDB_DRIVETRAIN_t* ) dataPtr ), entryHdr->flags & RDB_PKG_FLAG_EXTENDED, ident );
                break;
                    
            case RDB_PKG_ID_WHEEL:
                print( *( ( RDB_WHEEL_t* ) dataPtr ), entryHdr->flags & RDB_PKG_FLAG_EXTENDED, ident );
                break;

            case RDB_PKG_ID_PED_ANIMATION:
                print( *( ( RDB_PED_ANIMATION_t* ) dataPtr ), ident );
                break;

            case RDB_PKG_ID_SENSOR_STATE:
                print( *( ( RDB_SENSOR_STATE_t* ) dataPtr ), ident );
                break;

            case RDB_PKG_ID_SENSOR_OBJECT:
                print( *( ( RDB_SENSOR_OBJECT_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_CAMERA:
                print( *( ( RDB_CAMERA_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_CONTACT_POINT:
                print( *( ( RDB_CONTACT_POINT_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_TRAFFIC_SIGN:
                print( *( ( RDB_TRAFFIC_SIGN_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_ROAD_STATE:
                print( *( ( RDB_ROAD_STATE_t* ) dataPtr ), ident );
                break;
                  
            case RDB_PKG_ID_IMAGE:
            case RDB_PKG_ID_LIGHT_MAP:
                handleRDBitem( simTime, simFrame, *( ( RDB_IMAGE_t* ) dataPtr ) );
                break;
                   
            case RDB_PKG_ID_LIGHT_SOURCE:
                print( *( ( RDB_LIGHT_SOURCE_t* ) dataPtr ), entryHdr->flags & RDB_PKG_FLAG_EXTENDED, ident );
                break;
                    
            case RDB_PKG_ID_ENVIRONMENT:
                print( *( ( RDB_ENVIRONMENT_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_TRIGGER:
                print( *( ( RDB_TRIGGER_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_DRIVER_CTRL:
                print( *( ( RDB_DRIVER_CTRL_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_TRAFFIC_LIGHT:
                print( *( ( RDB_TRAFFIC_LIGHT_t* ) dataPtr ), entryHdr->flags & RDB_PKG_FLAG_EXTENDED, ident );
                break;
                    
            case RDB_PKG_ID_SYNC:
                print( *( ( RDB_SYNC_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_DRIVER_PERCEPTION:
                print( *( ( RDB_DRIVER_PERCEPTION_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_TONE_MAPPING:
                print( *( ( RDB_FUNCTION_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_ROAD_QUERY:
                print( *( ( RDB_ROAD_QUERY_t* ) dataPtr ), ident );
                break;
                    
            case RDB_PKG_ID_TRAJECTORY:
                print( *( ( RDB_TRAJECTORY_t* ) dataPtr ), ident );
                break;

            case RDB_PKG_ID_DYN_2_STEER:
                print( *( ( RDB_DYN_2_STEER_t* ) dataPtr ), ident );
                break;

            case RDB_PKG_ID_STEER_2_DYN:
                print( *( ( RDB_STEER_2_DYN_t* ) dataPtr ), ident );
                break;

            case RDB_PKG_ID_PROXY:
                print( *( ( RDB_PROXY_t* ) dataPtr ), ident );
                break;
*/
            default:
                printedMsg = false;
                break;
        }
        dataPtr += entryHdr->elementSize;
            
/*        if ( noElements && printedMsg )
            fprintf( stderr, "\n" );
            */
     }
}

void handleRDBitem( const double & simTime, const unsigned int & simFrame, RDB_OBJECT_STATE_t & item, bool isExtended )
{
    if (item.base.id == 1) // ego vehicel info
    {
        ego_info.length_ = item.base.geo.dimX;
        ego_info.width_ = item.base.geo.dimY;
        ego_info.x_ = item.base.pos.x;
        ego_info.y_ = item.base.pos.y;
        ego_info.speed_x_ = item.ext.speed.x;
        ego_info.speed_y_ = item.ext.speed.y;
        ego_info.speed_ = cos(item.base.pos.h) * item.ext.speed.x - sin(item.base.pos.h) * item.ext.speed.y;
        ego_info.absolute_theta_ = item.base.pos.h - 1.57;//relative to Y axis
        ego_info.acc_ = cos(item.base.pos.h) * item.ext.accel.x - sin(item.base.pos.h) * item.ext.accel.y;
    }
    if (item.base.id != 1) // other vehicel info
    {
        surVeh_info.push_back(item.base.pos.x);
        surVeh_info.push_back(item.base.pos.y);
        surVeh_info.push_back(item.base.geo.dimX);
        surVeh_info.push_back(item.base.geo.dimY);
        surVeh_info.push_back(item.ext.speed.x);
        surVeh_info.push_back(item.ext.speed.y);
        surVeh_info.push_back(item.ext.accel.x);
        surVeh_info.push_back(item.ext.accel.y);
        surVeh_info.push_back(item.base.pos.h - 1.57);//relative to Y axis
    }
}

void handleRDBitem( const double & simTime, const unsigned int & simFrame, RDB_ROAD_POS_t & item )
{
    if (item.playerId == 1)
        {
            ego_info.relative_theta_ = -item.hdgRel;
            ego_info.init_q_ = -(item.roadT-2);
        }
}

// void handleRDBitem( const double & simTime, const unsigned int & simFrame, RDB_IMAGE_t & item )
// {
//   fprintf( stderr, "handleRDBitem: handling image data\n" );
//   fprintf( stderr, "    simTime = %.3lf, simFrame = %ld\n", simTime, simFrame );
//   fprintf( stderr, "    id = %d, width = %d, height = %d\n", item.id, item.width, item.height );
//   fprintf( stderr, "    pixelSize = %d, pixelFormat = %d, imgSize = %d\n", item.pixelSize, item.pixelFormat, item.imgSize );

//   // write image to file
//   unsigned char* imgData = ( unsigned char* ) ( &item );
//   imgData += sizeof( RDB_IMAGE_t );
  
//   /* here could be a routine to process the image data, e.g.
//   processImgData( item.width, item.height, item.pixelSize, item.pixelFormat, item.imgSize, imgData, simFrame );
//   */
// }


void sendTrigger( int & sendSocket, const double & simTime, const unsigned int & simFrame )
{
  Framework::RDBHandler myHandler;

  myHandler.initMsg();

  RDB_TRIGGER_t *myTrigger = ( RDB_TRIGGER_t* ) myHandler.addPackage( simTime, simFrame, RDB_PKG_ID_TRIGGER );

  if ( !myTrigger )
    return;

  myTrigger->frameNo = simFrame + 1;
  myTrigger->deltaT  = 0.043;

  int retVal = send( sendSocket, ( const char* ) ( myHandler.getMsg() ), myHandler.getMsgTotalSize(), 0 );

  if ( !retVal )
    fprintf( stderr, "sendTrigger: could not send trigger\n" );

}


