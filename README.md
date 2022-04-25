# Life Game Simulator in C

![result](https://user-images.githubusercontent.com/63549742/146171699-2051680a-5aea-4c7e-94c3-4e943ac6c0c7.gif)


## Environment
- Ubuntu 20.04 LTS
- gcc version 9.3.0
- imagemagick (gif生成にのみ使用)
    ```
    $ convert --version
    Version: ImageMagick 6.9.10-23 Q16 x86_64 20190101 https://imagemagick.org
    Copyright: © 1999-2019 ImageMagick Studio LLC
    License: https://imagemagick.org/script/license.php
    Features: Cipher DPC Modules OpenMP 
    Delegates (built-in): bzlib djvu fftw fontconfig freetype jbig jng jpeg lcms lqr ltdl lzma openexr pangocairo png tiff webp wmf x xml zlib
    ```
- Mozilla Firefox 95.0

### imagemagickのインストール
#### Ubuntuの場合
```
sudo apt update
sudo apt install imagemagick
# 確認
convert --version
```
#### その他の場合
https://imagemagick.org/script/download.php を参照

### Firefoxのインストール
https://www.mozilla.org/en-US/firefox/new/ を参照


## Usage
### Build
```
# 実行に必要なソースを全てbuild
./build.sh
```

### Run
```
# データファイルを指定しない場合
./mylife
# データファイルを指定する場合
## Life1.06形式
./mylife data/gosperglidergun.lif
## RLE形式
./mylife data/gosperglidergun.rle
## plain txt形式
./mylife data/gosperglidergun.txt

# Gifの生成(任意)
./gen_gif.sh

# ログのクリア
./clear_logs.sh
```
#### シミュレーション中の操作
|key|動作|
|---|---|
|F|シミュレーション動作時は一時停止、一時停止時は一時停止解除|
|Q|終了|
|A|再生速度を1段階遅くする|
|D|再生速度を1段階早くする|

## Description
### wikiからダウンロードしてきたデータ(.txt形式)から.lif形式へ変換するコンバータ
テスト用のデータを増やすため、`.txt`から`.lif`へのコンバータ(`plaintxtToLife106.c`)を書いた。wikiにあるデータのほとんどは`.txt`及び`.rle`のため、このコンバータを使って3形式それぞれの読み取り関数のテストを行った(`test_parser.c`)
### 入力データとしてplain txt形式にも対応
`.txt`形式を直接パースできるようにした。
### 各入力形式のコメント行への対応
wikiの入力フォーマットの定義
- rle: https://www.conwaylife.com/wiki/Run_Length_Encoded
- lif: https://www.conwaylife.com/wiki/Life_1.06
- txt: https://www.conwaylife.com/wiki/Plaintext

に従って、rleとtxtに対してはコメント行の処理を行った。
rleに関しては、内容はひとまず"`log/fileinfo.txt`に書き出すことにした。
### extensionから読み込んで判定
各入力形式のコメント行にできるだけ対応するべく、ファイル形式の判定はヘッダの内容からではなく単純に拡張子から行った。
### Pose Menu機能
シミュレーション中に`f`を押すことでポーズメニューに以降し、再度`f`を押すことで再開するようにした。実装にはマルチスレッド(`pthread`)を用い、キー入力を待つ関数をthreadにわたして走らせた。フラグ変数のdata raceが生じないように`mutex`による排他制御にも気をつけた。

### 再生speedをリアルタイムに変更
シミュレーション中に`a`を押すことで再生速度低下、`d`を押すことで再生速度増加ができるようにした。7段階の再生速度を設けている。これもPose Menuと同様にmulti-threadingで行った。

### 生存割合のロギング
生存割合(`alive_ratio`)を`log/stats.csv`に各世代毎に記録した。

### .pbm形式での各世代の盤面の二値画像出力
[pbm形式](https://ja.wikipedia.org/wiki/PNM_(%E7%94%BB%E5%83%8F%E3%83%95%E3%82%A9%E3%83%BC%E3%83%9E%E3%83%83%E3%83%88)#PBM_%E3%81%AE%E4%BE%8B)の画像を書き出した。

### imagemagickを用いてgif化
pbm形式の画像の連番を用いて、`imagemagick`ライブラリの`convert`コマンドを使ってgif化した。Usageで述べたように`./gen_gif.sh`でgif化し、firefoxで開くようにしている。画像枚数が多すぎると(>=~800)エラーが出ることがある。

### Hash Tableによる盤面の記録と閉路検知
ライフゲームでは周期性が見られるため、これを記録するためにhash tableを使用した。
Hash Tableの実装にはReferenceに掲載した資料を参考にした。
盤面を01の文字列でエンコードし、その文字列に対してhash化を行うとした。
genが増えていく中で、毎回の盤面がhash tableに記録されているかどうかをチェックし、もし記録されていたら
- 閉路があるかどうか
- 閉路の開始点
- 閉路の長さ
を`/log/summary.txt`に記録した。

### 各種重要な部分へのtest
各種重要な関数は、あえて出力をテストしやすい形式にし、testを行った。
- `test_hash_table.c`
- `test_parser.c`

## Reference
- Life Wiki, https://www.conwaylife.com/wiki/Main_Page
-  Thomas H. Cormen, Charles E. Leiserson, Ronald L. Rivest, Clifford Stein, "Introduction to Algorithms Third Edition", MIT Press, 2009 
- Bennett Buchanan, "An Introduction to Hash Tables in C", Jul 11, 2016, https://medium.com/@bennettbuchanan/an-introduction-to-hash-tables-in-c-b83cbf2b4cf6

## Note
東京大学2021年度Aセメスター「ソフトウェア2」の発展課題として作成。
課題の都合上、ソースファイル単体での実装となっている。
