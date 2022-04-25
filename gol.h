/*
 ファイルによるセルの初期化: 生きているセルの座標が記述されたファイルをもとに2次元配列の状態を初期化する
 fp = NULL のときは、関数内で適宜定められた初期状態に初期化する。関数内初期値はdefault.lif と同じもの
 */
void init_cells(const int height, const int width, int cell[height][width], FILE* fp);

/*
 グリッドの描画: 世代情報とグリッドの配列等を受け取り、ファイルポインタに該当する出力にグリッドを描画する
 */
void print_cells(FILE *fp, int gen, const int height, const int width, int cell[height][width]);

/*
 着目するセルの周辺の生きたセルをカウントする関数
 */
int count_adjacent_cells(int h, int w, const int height, const int width, int cell[height][width]);

/*
 ライフゲームのルールに基づいて2次元配列の状態を更新する
 */
void update_cells(const int height, const int width, int cell[height][width]);
