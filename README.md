# TCP CORK serverプログラム

Linux独自のTCP_CORKオプションのマニュアル

https://man7.org/linux/man-pages/man7/tcp.7.html

> TCP_CORK (since Linux 2.2)
>
> If set, don't send out partial frames.  All queued partial
> frames are sent when the option is cleared again.  This is
> useful for prepending headers before calling sendfile(2),
> or for throughput optimization.  As currently implemented,
> there is a 200 millisecond ceiling on the time for which
> output is corked by TCP_CORK.  If this ceiling is reached,
> then queued data is automatically transmitted.  This
> option can be combined with TCP_NODELAY only since Linux
> 2.5.71.  This option should not be used in code intended
> to be portable.

## serverプログラムの起動方法

```
Usage: server [-p port] [-k] [-D] [-A]
-p port: port number (1234)
-k:      use TCP_CORK
-D:      use TCP_NODELAY
-A:      Accumurate each write() bytes
```

write()の動作は
100バイトのデータを連続10回write()する。
10回のwrite()後、sleep(1)で1秒休み。
これを5セット繰り返して終了。

``TCP_CORK``、``TCP_NODELAY``のオプションを設定する
タイミングについては下記を参照。

tcpdumpして、適当なクライアントで接続して、
何バイトのパケットが送られるか観察する。

## server -k (TCP_NODELAYの併用なし)

エラー処理を省略して書くと
```
for (int i = 0; i < 10; ++i) {
    unsigned char buf[100];
    set_so_cork(connfd, 1);
    int n = write(connfd, buf, sizeof(buf));
    set_so_cork(connfd, 0);
}
sleep(1);
```
のように
```
TCP_CORK セット
write(100バイト)
TCP_CORK アンセット
```
を10回繰り返し、1秒休み、を5セット繰り返したときの
サーバー側（送信側）でのtcpdumpの[ログ](cork-only.txt)

```
 1  0.000000 0.000000 IP 192.168.10.201.45628 > 192.168.10.200.1234: Flags [S], seq 2040766054, win 29200, options [mss 1460,nop,nop,sackOK], length 0
 2  0.000026 0.000026 IP 192.168.10.200.1234 > 192.168.10.201.45628: Flags [S.], seq 332823371, ack 2040766055, win 64240, options [mss 1460,nop,nop,sackOK], length 0
 3  0.000160 0.000134 IP 192.168.10.201.45628 > 192.168.10.200.1234: Flags [.], ack 1, win 29200, length 0
 4  0.000777 0.000617 IP 192.168.10.200.1234 > 192.168.10.201.45628: Flags [P.], seq 1:101, ack 1, win 64240, length 100
 5  0.000995 0.000218 IP 192.168.10.201.45628 > 192.168.10.200.1234: Flags [.], ack 101, win 29200, length 0
 6  0.001011 0.000016 IP 192.168.10.200.1234 > 192.168.10.201.45628: Flags [P.], seq 101:1001, ack 1, win 64240, length 900
 7  0.001252 0.000241 IP 192.168.10.201.45628 > 192.168.10.200.1234: Flags [.], ack 1001, win 30600, length 0
 8  1.001016 0.999764 IP 192.168.10.200.1234 > 192.168.10.201.45628: Flags [P.], seq 1001:1101, ack 1, win 64240, length 100
 9  1.001171 0.000155 IP 192.168.10.201.45628 > 192.168.10.200.1234: Flags [.], ack 1101, win 30600, length 0
10  1.001186 0.000015 IP 192.168.10.200.1234 > 192.168.10.201.45628: Flags [P.], seq 1101:2001, ack 1, win 64240, length 900
11  1.001386 0.000200 IP 192.168.10.201.45628 > 192.168.10.200.1234: Flags [.], ack 2001, win 32400, length 0
(以下略)

```
100バイトのパケットが送られることはない。

## server -k -D (TCP_CORKとTCP_NODELAYの併用)

エラー処理を省略して書くと
```
TCP_NODELAYをセット;
for (int i = 0; i < 10; ++i) {
    unsigned char buf[100];
    set_so_cork(connfd, 1);
    int n = write(connfd, buf, sizeof(buf));
    set_so_cork(connfd, 0);
}
sleep(1);
```
のときのtcpdumpの[ログ](cork-with-nodelay.txt)
```
  1  0.000000 0.000000 IP 192.168.10.201.41200 > 192.168.10.200.1234: Flags [S], seq 974911953, win 29200, options [mss 1460,nop,nop,sackOK], length 0
  2  0.000027 0.000027 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [S.], seq 485040323, ack 974911954, win 64240, options [mss 1460,nop,nop,sackOK], length 0
  3  0.000299 0.000272 IP 192.168.10.201.41200 > 192.168.10.200.1234: Flags [.], ack 1, win 29200, length 0
  4  0.000848 0.000549 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 1:101, ack 1, win 64240, length 100
  5  0.000861 0.000013 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 101:201, ack 1, win 64240, length 100
  6  0.000872 0.000011 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 201:301, ack 1, win 64240, length 100
  7  0.000881 0.000009 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 301:401, ack 1, win 64240, length 100
  8  0.000891 0.000010 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 401:501, ack 1, win 64240, length 100
  9  0.000900 0.000009 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 501:601, ack 1, win 64240, length 100
 10  0.000910 0.000010 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 601:701, ack 1, win 64240, length 100
 11  0.000921 0.000011 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 701:801, ack 1, win 64240, length 100
 12  0.000931 0.000010 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 801:901, ack 1, win 64240, length 100
 13  0.000940 0.000009 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 901:1001, ack 1, win 64240, length 100
 14  0.001104 0.000164 IP 192.168.10.201.41200 > 192.168.10.200.1234: Flags [.], ack 101, win 29200, length 0
 15  0.001105 0.000001 IP 192.168.10.201.41200 > 192.168.10.200.1234: Flags [.], ack 201, win 29200, length 0
 16  0.001105 0.000000 IP 192.168.10.201.41200 > 192.168.10.200.1234: Flags [.], ack 401, win 30016, length 0
 17  0.001106 0.000001 IP 192.168.10.201.41200 > 192.168.10.200.1234: Flags [.], ack 801, win 31088, length 0
 18  0.001107 0.000001 IP 192.168.10.201.41200 > 192.168.10.200.1234: Flags [.], ack 1001, win 32160, length 0
 19  1.001088 0.999981 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 1001:1101, ack 1, win 64240, length 100
 20  1.001104 0.000016 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 1101:1201, ack 1, win 64240, length 100
 21  1.001116 0.000012 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 1201:1301, ack 1, win 64240, length 100
 22  1.001126 0.000010 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 1301:1401, ack 1, win 64240, length 100
 23  1.001136 0.000010 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 1401:1501, ack 1, win 64240, length 100
 24  1.001146 0.000010 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 1501:1601, ack 1, win 64240, length 100
 25  1.001156 0.000010 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 1601:1701, ack 1, win 64240, length 100
 26  1.001166 0.000010 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 1701:1801, ack 1, win 64240, length 100
 27  1.001176 0.000010 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 1801:1901, ack 1, win 64240, length 100
 28  1.001186 0.000010 IP 192.168.10.200.1234 > 192.168.10.201.41200: Flags [P.], seq 1901:2001, ack 1, win 64240, length 100
 29  1.001199 0.000013 IP 192.168.10.201.41200 > 192.168.10.200.1234: Flags [.], ack 1101, win 32160, length 0
 30  1.001200 0.000001 IP 192.168.10.201.41200 > 192.168.10.200.1234: Flags [.], ack 1201, win 32160, length 0
 31  1.001258 0.000058 IP 192.168.10.201.41200 > 192.168.10.200.1234: Flags [.], ack 1301, win 32160, length 0
 32  1.001259 0.000001 IP 192.168.10.201.41200 > 192.168.10.200.1234: Flags [.], ack 1801, win 33232, length 0
 33  1.001285 0.000026 IP 192.168.10.201.41200 > 192.168.10.200.1234: Flags [.], ack 1901, win 33232, length 0
 34  1.001286 0.000001 IP 192.168.10.201.41200 > 192.168.10.200.1234: Flags [.], ack 2001, win 33232, length 0
```
のようにwrite(100バイト)するごとにパケットが送られるようになる。

### server -A -k (-D)で起動してwrite()したデータをためてから送信する

エラー処理なしで書くと
```
set_so_cork(connfd, 1); /* prepare for accumulation */
for (int i = 0; i < 10; ++i) {
    unsigned char buf[100];
    int n = write(connfd, buf, sizeof(buf));
    if (n < 0) {
        err(1, "write");
    }
}
set_so_cork(connfd, 0); /* flush */
sleep(1);
のようにwrite(100バイト)を貯めておいて、1回のwrite()で
ためた分を送信することができる。

tcpdumpの[ログ](accumulate.txt)
```
1  0.000000 0.000000 IP 192.168.10.201.47830 > 192.168.10.200.1234: Flags [S], seq 2005752396, win 29200, options [mss 1460,nop,nop,sackOK], length 0
2  0.000025 0.000025 IP 192.168.10.200.1234 > 192.168.10.201.47830: Flags [S.], seq 2199698152, ack 2005752397, win 64240, options [mss 1460,nop,nop,sackOK], length 0
3  0.000300 0.000275 IP 192.168.10.201.47830 > 192.168.10.200.1234: Flags [.], ack 1, win 29200, length 0
4  0.000908 0.000608 IP 192.168.10.200.1234 > 192.168.10.201.47830: Flags [P.], seq 1:1001, ack 1, win 64240, length 1000
5  0.001035 0.000127 IP 192.168.10.201.47830 > 192.168.10.200.1234: Flags [.], ack 1001, win 31000, length 0
6  1.001151 1.000116 IP 192.168.10.200.1234 > 192.168.10.201.47830: Flags [P.], seq 1001:2001, ack 1, win 64240, length 1000
7  1.001387 0.000236 IP 192.168.10.201.47830 > 192.168.10.200.1234: Flags [.], ack 2001, win 33000, length 0
```
4行め、5行め: write(100バイト) 10回のデータが蓄積されたあとTCP_CORKを0にすることで
送信されている。

server -A -kの場合は-Dを追加してTCP_NODELAYを有効にしても、
-DをつけないでTCP_NODELAYを無効な状態のままにしていても1000バイトの
パケットが送られてくることにかわりはない。

### writev()

ヘッダを送ってそのあとにデータ本体を送るなど違うバッファに
格納されているデータを送るときはTCP_CORKでためるよりも
writev()を使って書くという方法もある(writev()のほうがいいかもしれない)。
