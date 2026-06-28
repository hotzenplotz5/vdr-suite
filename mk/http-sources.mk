HTTP_CLIENT_SRC := \
        core/http/src/BasicHttpClient.cpp

HTTP_LISTENER_SRC := \
        core/http/src/SimpleHttpListener.cpp

HTTP_TEST_SUPPORT_SRC := \
        core/http/src/MockHttpClient.cpp \
        core/http/src/TestHttpServer.cpp

# Transitional compatibility aggregate for existing tests and build targets.
HTTP_SRC := \
        core/http/src/MockHttpClient.cpp \

# Transitional compatibility aggregate for existing server/harness targets.
HTTP_SERVER_SRC := \
        core/http/src/BasicHttpClient.cpp \
        core/http/src/SimpleHttpListener.cpp \
        core/http/src/TestHttpServer.cpp
