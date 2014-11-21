注意事项：
1.auto_dtcc++.sh 该脚本是需要root权限去执行的，它的作用是将dtc sdk 的动态库
以及 php 扩展动态库（该动态库依赖于dtc c++ 的动态库）放置在 /usr/lib 目录下，
并且执行命令 ldconfig 使配置生效
2.执行脚本之后，可以在 php/bin 目录下执行 ./php -m 查看是否有 tphp_dtc 的扩展