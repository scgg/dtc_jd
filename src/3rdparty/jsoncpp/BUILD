# pecywang

cc_library
(
    name = 'jsoncpp',
    srcs = [
        'src/json_reader.cpp',
        'src/json_value.cpp',
        'src/json_writer.cpp'
    ],
    incs = [
        ':include'
    ]
)

cc_binary
(
    name = 'test_jsoncpp',
    srcs = [
        'test_client/test_jsoncpp.cpp'
    ],
    deps = [
        ':jsoncpp'
    ]
)