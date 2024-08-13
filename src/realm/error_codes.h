/*************************************************************************
 *
 * Copyright 2022 Realm Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************/

#ifndef REALM_ERROR_CODES_H
#define REALM_ERROR_CODES_H

typedef enum realm_error_category {
    RLM_ERR_CAT_LOGIC = 0x0002,
    RLM_ERR_CAT_RUNTIME = 0x0004,
    RLM_ERR_CAT_INVALID_ARG = 0x0008,
    RLM_ERR_CAT_FILE_ACCESS = 0x0010,
    RLM_ERR_CAT_SYSTEM_ERROR = 0x0020,
    RLM_ERR_CAT_APP_ERROR = 0x0040,
    RLM_ERR_CAT_CLIENT_ERROR = 0x0080,
    RLM_ERR_CAT_JSON_ERROR = 0x0100,
    RLM_ERR_CAT_SERVICE_ERROR = 0x0200,
    RLM_ERR_CAT_HTTP_ERROR = 0x0400,
    RLM_ERR_CAT_CUSTOM_ERROR = 0x0800,
    RLM_ERR_CAT_WEBSOCKET_ERROR = 0x1000,
    RLM_ERR_CAT_SYNC_ERROR = 0x2000,
} realm_error_category_e;

typedef enum realm_errno {
    RLM_ERR_NONE = 0,
    RLM_ERR_RUNTIME = 1000,
    RLM_ERR_RANGE_ERROR = 1001,
    RLM_ERR_BROKEN_INVARIANT = 1002,
    RLM_ERR_OUT_OF_MEMORY = 1003,
    RLM_ERR_OUT_OF_DISK_SPACE = 1004,
    RLM_ERR_ADDRESS_SPACE_EXHAUSTED = 1005,
    RLM_ERR_MAXIMUM_FILE_SIZE_EXCEEDED = 1006,
    RLM_ERR_INCOMPATIBLE_SESSION = 1007,
    RLM_ERR_INCOMPATIBLE_LOCK_FILE = 1008,
    RLM_ERR_INVALID_QUERY = 1009,
    RLM_ERR_BAD_VERSION = 1010,
    RLM_ERR_UNSUPPORTED_FILE_FORMAT_VERSION = 1011,
    RLM_ERR_MULTIPLE_SYNC_AGENTS = 1012,
    RLM_ERR_OBJECT_ALREADY_EXISTS = 1013,
    RLM_ERR_NOT_CLONABLE = 1014,
    RLM_ERR_BAD_CHANGESET = 1015,
    RLM_ERR_SUBSCRIPTION_FAILED = 1016,
    RLM_ERR_FILE_OPERATION_FAILED = 1017,
    RLM_ERR_FILE_PERMISSION_DENIED = 1018,
    RLM_ERR_FILE_NOT_FOUND = 1019,
    RLM_ERR_FILE_ALREADY_EXISTS = 1020,
    RLM_ERR_INVALID_DATABASE = 1021,
    RLM_ERR_DECRYPTION_FAILED = 1022,
    RLM_ERR_INCOMPATIBLE_HISTORIES = 1023,
    RLM_ERR_FILE_FORMAT_UPGRADE_REQUIRED = 1024,
    RLM_ERR_SCHEMA_VERSION_MISMATCH = 1025,
    RLM_ERR_NO_SUBSCRIPTION_FOR_WRITE = 1026,
    RLM_ERR_OPERATION_ABORTED = 1027,

    RLM_ERR_AUTO_CLIENT_RESET_FAILED = 1028,
    RLM_ERR_BAD_SYNC_PARTITION_VALUE = 1029,
    RLM_ERR_CONNECTION_CLOSED = 1030,
    RLM_ERR_INVALID_SUBSCRIPTION_QUERY = 1031,
    RLM_ERR_SYNC_CLIENT_RESET_REQUIRED = 1032,
    RLM_ERR_SYNC_COMPENSATING_WRITE = 1033,
    RLM_ERR_SYNC_CONNECT_FAILED = 1034,
    RLM_ERR_SYNC_CONNECT_TIMEOUT = 1035,
    RLM_ERR_SYNC_INVALID_SCHEMA_CHANGE = 1036,
    RLM_ERR_SYNC_PERMISSION_DENIED = 1037,
    RLM_ERR_SYNC_PROTOCOL_INVARIANT_FAILED = 1038,
    RLM_ERR_SYNC_PROTOCOL_NEGOTIATION_FAILED = 1039,
    RLM_ERR_SYNC_SERVER_PERMISSIONS_CHANGED = 1040,
    RLM_ERR_SYNC_USER_MISMATCH = 1041,
    RLM_ERR_TLS_HANDSHAKE_FAILED = 1042,
    RLM_ERR_WRONG_SYNC_TYPE = 1043,
    RLM_ERR_SYNC_WRITE_NOT_ALLOWED = 1044,
    RLM_ERR_SYNC_LOCAL_CLOCK_BEFORE_EPOCH = 1045,
    RLM_ERR_SYNC_SCHEMA_MIGRATION_ERROR = 1046,

    RLM_ERR_SYSTEM_ERROR = 1999,

    RLM_ERR_LOGIC = 2000,
    RLM_ERR_NOT_SUPPORTED = 2001,
    RLM_ERR_BROKEN_PROMISE = 2002,
    RLM_ERR_CROSS_TABLE_LINK_TARGET = 2003,
    RLM_ERR_KEY_ALREADY_USED = 2004,
    RLM_ERR_WRONG_TRANSACTION_STATE = 2005,
    RLM_ERR_WRONG_THREAD = 2006,
    RLM_ERR_ILLEGAL_OPERATION = 2007,
    RLM_ERR_SERIALIZATION_ERROR = 2008,
    RLM_ERR_STALE_ACCESSOR = 2009,
    RLM_ERR_INVALIDATED_OBJECT = 2010,
    RLM_ERR_READ_ONLY_DB = 2011,
    RLM_ERR_DELETE_OPENED_REALM = 2012,
    RLM_ERR_MISMATCHED_CONFIG = 2013,
    RLM_ERR_CLOSED_REALM = 2014,
    RLM_ERR_INVALID_TABLE_REF = 2015,
    RLM_ERR_SCHEMA_VALIDATION_FAILED = 2016,
    RLM_ERR_SCHEMA_MISMATCH = 2017,
    RLM_ERR_INVALID_SCHEMA_VERSION = 2018,
    RLM_ERR_INVALID_SCHEMA_CHANGE = 2019,
    RLM_ERR_MIGRATION_FAILED = 2020,
    RLM_ERR_TOP_LEVEL_OBJECT = 2021,

    RLM_ERR_INVALID_ARGUMENT = 3000,
    RLM_ERR_PROPERTY_TYPE_MISMATCH = 3001,
    RLM_ERR_PROPERTY_NOT_NULLABLE = 3002,
    RLM_ERR_READ_ONLY_PROPERTY = 3003,
    RLM_ERR_MISSING_PROPERTY_VALUE = 3004,
    RLM_ERR_MISSING_PRIMARY_KEY = 3005,
    RLM_ERR_UNEXPECTED_PRIMARY_KEY = 3006,
    RLM_ERR_MODIFY_PRIMARY_KEY = 3007,
    RLM_ERR_INVALID_QUERY_STRING = 3008,
    RLM_ERR_INVALID_PROPERTY = 3009,
    RLM_ERR_INVALID_NAME = 3010,
    RLM_ERR_INVALID_DICTIONARY_KEY = 3011,
    RLM_ERR_INVALID_DICTIONARY_VALUE = 3012,
    RLM_ERR_INVALID_SORT_DESCRIPTOR = 3013,
    RLM_ERR_INVALID_ENCRYPTION_KEY = 3014,
    RLM_ERR_INVALID_QUERY_ARG = 3015,
    RLM_ERR_NO_SUCH_OBJECT = 3016,
    RLM_ERR_INDEX_OUT_OF_BOUNDS = 3017,
    RLM_ERR_LIMIT_EXCEEDED = 3018,
    RLM_ERR_OBJECT_TYPE_MISMATCH = 3019,
    RLM_ERR_NO_SUCH_TABLE = 3020,
    RLM_ERR_TABLE_NAME_IN_USE = 3021,
    RLM_ERR_ILLEGAL_COMBINATION = 3022,
    RLM_ERR_BAD_SERVER_URL = 3023,

    RLM_ERR_CUSTOM_ERROR = 4000,

    RLM_ERR_CLIENT_USER_NOT_FOUND = 4100,
    RLM_ERR_CLIENT_USER_NOT_LOGGED_IN = 4101,
    RLM_ERR_CLIENT_REDIRECT_ERROR = 4103,
    RLM_ERR_CLIENT_TOO_MANY_REDIRECTS = 4104,
    RLM_ERR_CLIENT_USER_ALREADY_NAMED = 4105,

    RLM_ERR_BAD_TOKEN = 4200,
    RLM_ERR_MALFORMED_JSON = 4201,
    RLM_ERR_MISSING_JSON_KEY = 4202,
    RLM_ERR_BAD_BSON_PARSE = 4203,

    RLM_ERR_MISSING_AUTH_REQ = 4300,
    RLM_ERR_INVALID_SESSION = 4301,
    RLM_ERR_USER_APP_DOMAIN_MISMATCH = 4302,
    RLM_ERR_DOMAIN_NOT_ALLOWED = 4303,
    RLM_ERR_READ_SIZE_LIMIT_EXCEEDED = 4304,
    RLM_ERR_INVALID_PARAMETER = 4305,
    RLM_ERR_MISSING_PARAMETER = 4306,
    RLM_ERR_TWILIO_ERROR = 4307,
    RLM_ERR_GCM_ERROR = 4308,
    RLM_ERR_HTTP_ERROR = 4309,
    RLM_ERR_AWS_ERROR = 4310,
    RLM_ERR_MONGODB_ERROR = 4311,
    RLM_ERR_ARGUMENTS_NOT_ALLOWED = 4312,
    RLM_ERR_FUNCTION_EXECUTION_ERROR = 4313,
    RLM_ERR_NO_MATCHING_RULE_FOUND = 4314,
    RLM_ERR_INTERNAL_SERVER_ERROR = 4315,
    RLM_ERR_AUTH_PROVIDER_NOT_FOUND = 4316,
    RLM_ERR_AUTH_PROVIDER_ALREADY_EXISTS = 4317,
    RLM_ERR_SERVICE_NOT_FOUND = 4318,
    RLM_ERR_SERVICE_TYPE_NOT_FOUND = 4319,
    RLM_ERR_SERVICE_ALREADY_EXISTS = 4320,
    RLM_ERR_SERVICE_COMMAND_NOT_FOUND = 4321,
    RLM_ERR_VALUE_NOT_FOUND = 4322,
    RLM_ERR_VALUE_ALREADY_EXISTS = 4323,
    RLM_ERR_VALUE_DUPLICATE_NAME = 4324,
    RLM_ERR_FUNCTION_NOT_FOUND = 4325,
    RLM_ERR_FUNCTION_ALREADY_EXISTS = 4326,
    RLM_ERR_FUNCTION_DUPLICATE_NAME = 4327,
    RLM_ERR_FUNCTION_SYNTAX_ERROR = 4328,
    RLM_ERR_FUNCTION_INVALID = 4329,
    RLM_ERR_INCOMING_WEBHOOK_NOT_FOUND = 4330,
    RLM_ERR_INCOMING_WEBHOOK_ALREADY_EXISTS = 4331,
    RLM_ERR_INCOMING_WEBHOOK_DUPLICATE_NAME = 4332,
    RLM_ERR_RULE_NOT_FOUND = 4333,
    RLM_ERR_API_KEY_NOT_FOUND = 4334,
    RLM_ERR_RULE_ALREADY_EXISTS = 4335,
    RLM_ERR_RULE_DUPLICATE_NAME = 4336,
    RLM_ERR_AUTH_PROVIDER_DUPLICATE_NAME = 4337,
    RLM_ERR_RESTRICTED_HOST = 4338,
    RLM_ERR_API_KEY_ALREADY_EXISTS = 4339,
    RLM_ERR_INCOMING_WEBHOOK_AUTH_FAILED = 4340,
    RLM_ERR_EXECUTION_TIME_LIMIT_EXCEEDED = 4341,
    RLM_ERR_NOT_CALLABLE = 4342,
    RLM_ERR_USER_ALREADY_CONFIRMED = 4343,
    RLM_ERR_USER_NOT_FOUND = 4344,
    RLM_ERR_USER_DISABLED = 4345,
    RLM_ERR_AUTH_ERROR = 4346,
    RLM_ERR_BAD_REQUEST = 4347,
    RLM_ERR_ACCOUNT_NAME_IN_USE = 4348,
    RLM_ERR_INVALID_PASSWORD = 4349,
    RLM_ERR_SCHEMA_VALIDATION_FAILED_WRITE = 4350,
    RLM_ERR_APP_UNKNOWN = 4351,
    RLM_ERR_MAINTENANCE_IN_PROGRESS = 4352,
    RLM_ERR_USERPASS_TOKEN_INVALID = 4353,
    RLM_ERR_INVALID_SERVER_RESPONSE = 4354,
    RLM_ERR_APP_SERVER_ERROR = 4355,

    RLM_ERR_CALLBACK = 1000000, /**< A user-provided callback failed. */
    RLM_ERR_UNKNOWN = 2000000   /* Should not be used in code */
} realm_errno_e;

typedef enum realm_sync_errno_connection {
    RLM_SYNC_ERR_CONNECTION_CONNECTION_CLOSED = 100,
    RLM_SYNC_ERR_CONNECTION_OTHER_ERROR = 101,
    RLM_SYNC_ERR_CONNECTION_UNKNOWN_MESSAGE = 102,
    RLM_SYNC_ERR_CONNECTION_BAD_SYNTAX = 103,
    RLM_SYNC_ERR_CONNECTION_LIMITS_EXCEEDED = 104,
    RLM_SYNC_ERR_CONNECTION_WRONG_PROTOCOL_VERSION = 105,
    RLM_SYNC_ERR_CONNECTION_BAD_SESSION_IDENT = 106,
    RLM_SYNC_ERR_CONNECTION_REUSE_OF_SESSION_IDENT = 107,
    RLM_SYNC_ERR_CONNECTION_BOUND_IN_OTHER_SESSION = 108,
    RLM_SYNC_ERR_CONNECTION_BAD_MESSAGE_ORDER = 109,
    RLM_SYNC_ERR_CONNECTION_BAD_DECOMPRESSION = 110,
    RLM_SYNC_ERR_CONNECTION_BAD_CHANGESET_HEADER_SYNTAX = 111,
    RLM_SYNC_ERR_CONNECTION_BAD_CHANGESET_SIZE = 112,
    RLM_SYNC_ERR_CONNECTION_SWITCH_TO_FLX_SYNC = 113,
    RLM_SYNC_ERR_CONNECTION_SWITCH_TO_PBS = 114,
    // Error code 199 is reserved as an "unknown connection error" in tests
} realm_sync_errno_connection_e;

typedef enum realm_sync_errno_session {
    RLM_SYNC_ERR_SESSION_SESSION_CLOSED = 200,
    RLM_SYNC_ERR_SESSION_OTHER_SESSION_ERROR = 201,
    RLM_SYNC_ERR_SESSION_TOKEN_EXPIRED = 202,
    RLM_SYNC_ERR_SESSION_BAD_AUTHENTICATION = 203,
    RLM_SYNC_ERR_SESSION_ILLEGAL_REALM_PATH = 204,
    RLM_SYNC_ERR_SESSION_NO_SUCH_REALM = 205,
    RLM_SYNC_ERR_SESSION_PERMISSION_DENIED = 206,
    RLM_SYNC_ERR_SESSION_BAD_SERVER_FILE_IDENT = 207,
    RLM_SYNC_ERR_SESSION_BAD_CLIENT_FILE_IDENT = 208,
    RLM_SYNC_ERR_SESSION_BAD_SERVER_VERSION = 209,
    RLM_SYNC_ERR_SESSION_BAD_CLIENT_VERSION = 210,
    RLM_SYNC_ERR_SESSION_DIVERGING_HISTORIES = 211,
    RLM_SYNC_ERR_SESSION_BAD_CHANGESET = 212,
    RLM_SYNC_ERR_SESSION_PARTIAL_SYNC_DISABLED = 214,
    RLM_SYNC_ERR_SESSION_UNSUPPORTED_SESSION_FEATURE = 215,
    RLM_SYNC_ERR_SESSION_BAD_ORIGIN_FILE_IDENT = 216,
    RLM_SYNC_ERR_SESSION_BAD_CLIENT_FILE = 217,
    RLM_SYNC_ERR_SESSION_SERVER_FILE_DELETED = 218,
    RLM_SYNC_ERR_SESSION_CLIENT_FILE_BLACKLISTED = 219,
    RLM_SYNC_ERR_SESSION_USER_BLACKLISTED = 220,
    RLM_SYNC_ERR_SESSION_TRANSACT_BEFORE_UPLOAD = 221,
    RLM_SYNC_ERR_SESSION_CLIENT_FILE_EXPIRED = 222,
    RLM_SYNC_ERR_SESSION_USER_MISMATCH = 223,
    RLM_SYNC_ERR_SESSION_TOO_MANY_SESSIONS = 224,
    RLM_SYNC_ERR_SESSION_INVALID_SCHEMA_CHANGE = 225,
    RLM_SYNC_ERR_SESSION_BAD_QUERY = 226,
    RLM_SYNC_ERR_SESSION_OBJECT_ALREADY_EXISTS = 227,
    RLM_SYNC_ERR_SESSION_SERVER_PERMISSIONS_CHANGED = 228,
    RLM_SYNC_ERR_SESSION_INITIAL_SYNC_NOT_COMPLETED = 229,
    RLM_SYNC_ERR_SESSION_WRITE_NOT_ALLOWED = 230,
    RLM_SYNC_ERR_SESSION_COMPENSATING_WRITE = 231,
    RLM_SYNC_ERR_SESSION_MIGRATE_TO_FLX = 232,
    RLM_SYNC_ERR_SESSION_BAD_PROGRESS = 233,
    RLM_SYNC_ERR_SESSION_REVERT_TO_PBS = 234,
    RLM_SYNC_ERR_SESSION_BAD_SCHEMA_VERSION = 235,
    RLM_SYNC_ERR_SESSION_SCHEMA_VERSION_CHANGED = 236,
    // Error code 237 is not used by the client
    // Error code 238 is not used by the sync protocol
    RLM_SYNC_ERR_SESSION_SCHEMA_VERSION_FORCE_UPGRADE = 239,
    // Error code 299 is reserved as an "unknown session error" in tests
} realm_sync_errno_session_e;

typedef enum realm_web_socket_errno {
    RLM_ERR_WEBSOCKET_OK = 1000,
    RLM_ERR_WEBSOCKET_GOINGAWAY = 1001,
    RLM_ERR_WEBSOCKET_PROTOCOLERROR = 1002,
    RLM_ERR_WEBSOCKET_UNSUPPORTEDDATA = 1003,
    RLM_ERR_WEBSOCKET_RESERVED = 1004,
    RLM_ERR_WEBSOCKET_NOSTATUSRECEIVED = 1005,
    RLM_ERR_WEBSOCKET_ABNORMALCLOSURE = 1006,
    RLM_ERR_WEBSOCKET_INVALIDPAYLOADDATA = 1007,
    RLM_ERR_WEBSOCKET_POLICYVIOLATION = 1008,
    RLM_ERR_WEBSOCKET_MESSAGETOOBIG = 1009,
    RLM_ERR_WEBSOCKET_INAVALIDEXTENSION = 1010,
    RLM_ERR_WEBSOCKET_INTERNALSERVERERROR = 1011,
    RLM_ERR_WEBSOCKET_TLSHANDSHAKEFAILED = 1015,

    RLM_ERR_WEBSOCKET_UNAUTHORIZED = 4001,
    RLM_ERR_WEBSOCKET_FORBIDDEN = 4002,
    RLM_ERR_WEBSOCKET_MOVEDPERMANENTLY = 4003,

    RLM_ERR_WEBSOCKET_RESOLVE_FAILED = 4400,
    RLM_ERR_WEBSOCKET_CONNECTION_FAILED = 4401,
    RLM_ERR_WEBSOCKET_READ_ERROR = 4402,
    RLM_ERR_WEBSOCKET_WRITE_ERROR = 4403,
    RLM_ERR_WEBSOCKET_RETRY_ERROR = 4404,
    RLM_ERR_WEBSOCKET_FATAL_ERROR = 4405,
} realm_web_socket_errno_e;

typedef enum realm_sync_socket_callback_result {
    // These error values are pulled directly from realm_errno_e
    RLM_ERR_SYNC_SOCKET_SUCCESS = RLM_ERR_NONE,
    RLM_ERR_SYNC_SOCKET_OPERATION_ABORTED = RLM_ERR_OPERATION_ABORTED,
    RLM_ERR_SYNC_SOCKET_RUNTIME = RLM_ERR_RUNTIME,
    RLM_ERR_SYNC_SOCKET_OUT_OF_MEMORY = RLM_ERR_OUT_OF_MEMORY,
    RLM_ERR_SYNC_SOCKET_ADDRESS_SPACE_EXHAUSTED = RLM_ERR_ADDRESS_SPACE_EXHAUSTED,
    RLM_ERR_SYNC_SOCKET_CONNECTION_CLOSED = RLM_ERR_CONNECTION_CLOSED,
    RLM_ERR_SYNC_SOCKET_NOT_SUPPORTED = RLM_ERR_NOT_SUPPORTED,
    RLM_ERR_SYNC_SOCKET_INVALID_ARGUMENT = RLM_ERR_INVALID_ARGUMENT,
} realm_sync_socket_callback_result_e;

#endif /* REALM_ERROR_CODES_H */
