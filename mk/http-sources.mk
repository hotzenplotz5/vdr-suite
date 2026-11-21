HTTP_CLIENT_SRC := \
        core/http/src/BasicHttpClient.cpp

HTTP_LISTENER_SRC := \
        core/http/src/SimpleHttpListener.cpp

HTTP_TEST_SUPPORT_SRC := \
        core/http/src/MockHttpClient.cpp \
        core/http/src/TestHttpServer.cpp

# Transitional compatibility aggregate for existing tests and build targets.
HTTP_SRC := \
        $(HTTP_TEST_SUPPORT_SRC)

# Transitional compatibility aggregate for existing server/harness targets.
HTTP_SERVER_SRC := \
        $(HTTP_CLIENT_SRC) \
        $(HTTP_LISTENER_SRC) \
        core/http/src/TestHttpServer.cpp
