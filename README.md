# serverプログラム
```
Usage: server [-p port] [-k] [-D] 
-p port: port number (1234)
-k:      use TCP_CORK
-D:      use TCP_NODELAY
```

100バイトのデータを連続10回write()する。
10回のwrite()後、sleep(1)で1秒休み。
これを5セット繰り返して終了。

tcpdumpして、適当なクライアントで接続して、
何バイトのパケットが送られるか観察する。
