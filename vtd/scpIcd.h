/*****************************************************//**
 * @file
 * ICD of the Simulation Control Protocol (SCP)
 *
 * (c) VIRES GmbH
 * @author Marius Dupuis
 ********************************************************/
/**
   @page SCP_CHANGE_LOG SCP Change Log
-  04.10.2013: re-formatted to comply with RDB header file
-  22.04.2010: modified for 64bit compatibility
-  25.08.2008: added pragma instructions
-  04.01.2008: created
*********************************************************/

#pragma pack (push, 4)

#ifndef _SCP_ICD_H
#define _SCP_ICD_H

/* ====== INCLUDES ====== */
#include <stdint.h>

/** @addtogroup GENERAL_DEFINITIONS
 *  @{
 */
#define SCP_DEFAULT_PORT 48179   /**< default port for SCP communication        @version 0x0001 */
#define SCP_NAME_LENGTH  64      /**< length of a name sent via SCP             @version 0x0001 */
#define SCP_MAGIC_NO     40108   /**< magic number                              @version 0x0001 */
#define SCP_VERSION      0x0001  /**< upper byte = major, lower byte = minor    @version 0x0001 */
/** @} */

/**
*   the SCP message header
*/
typedef struct
{
    uint16_t  magicNo;                    /**< must be SCP_MAGIC_NO                         @unit @link GENERAL_DEFINITIONS @endlink   @version 0x0001 */
    uint16_t  version;                    /**< upper byte = major, lower byte = minor       @unit _                                    @version 0x0001 */
    char      sender[SCP_NAME_LENGTH];    /**< name of the sender as text                   @unit _                                    @version 0x0001 */
    char      receiver[SCP_NAME_LENGTH];  /**< name of the receiver as text                 @unit _                                    @version 0x0001 */
    uint32_t  dataSize;                   /**< number of data bytes following the header    @unit byte                                 @version 0x0001 */
} SCP_MSG_HDR_t;

#endif		/* _SCP_ICD_H */

// end of pragma 4
#pragma pack(pop)


