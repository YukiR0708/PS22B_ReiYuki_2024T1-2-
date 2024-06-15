# include <Siv3D.hpp>
/*
	よりC++ライクな書き方
	・クラスベース
	・継承を行う
*/


enum class State
{
	Title,
	Game,
	GameOver,
	Clear,
};

using GameManager = SceneManager<State>;

class ScoreManager;
class Ball;
class Bricks;
class Paddle;
class StretchItem;
class ItemSpawner;
class GameOver;
class Clear;

///@brief 点数処理
class  ScoreManager final {
private:
	///@brief 加算するスコア
	uint16 scoreToAdd = 10;
	const Font scoreFont{ 50, Typeface::Bold };

public:
	static uint32& GetScore() {
		static uint32 score = 0;
		return score;
	}

	static void AddScore(uint32 scoreToAdd) {
		GetScore() += scoreToAdd;
	}

	void Draw()const {
		scoreFont(U"Score:", GetScore()).draw(24, 20, 20, Color(30, 26, 27));
	}
};

class Title : public GameManager::Scene
{
public:

	/// @brief コンストラクタ
	/// @param init 
	Title(const InitData& init) : IScene{ init } {
		ScoreManager::GetScore() = 0;
	}

	void update() override
	{
		m_startTransition.update(m_startButton.mouseOver());
		m_exitTransition.update(m_exitButton.mouseOver());

		if (m_startButton.mouseOver() || m_exitButton.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}

		if (m_startButton.leftClicked())
		{
			// ゲームシーンへ
			submit.playOneShot();
			changeScene(State::Game);
		}
		else if (m_exitButton.leftClicked())
		{
			// 終了
			submit.playOneShot();
			System::Exit();
		}
	}

	void draw() const override
	{
		//ボタン描画
		m_startButton.draw(ColorF(U"#51fbed").setA(m_startTransition.value())).drawFrame(1, Color(30, 26, 27));
		m_exitButton.draw(ColorF(U"#51fbed").setA(m_exitTransition.value())).drawFrame(1, Color(30, 26, 27));

		//文字描画
		TitleFont(U"Block Shoot!!").drawAt(Scene::Center().x, Scene::Center().y - 80, Color(30, 26, 27));
		StartFont(U"Go to Game").drawAt(m_startButton.center(), Color(157, 21, 36));
		ExitFont(U"Exit").drawAt(m_exitButton.center(), Color(157, 21, 36));
	}

private:

	/// @brief タイトル・ボタンの描画に関わるメンバ変数
	Rect m_startButton{ Arg::center(Scene::Center().x, Scene::Center().y + 80), 300, 60 };
	Transition m_startTransition{ 0.4s, 0.2s };
	Rect m_exitButton{ Arg::center = Scene::Center().movedBy(0, 200), 300, 60 };
	Transition m_exitTransition{ 0.4s, 0.2s };
	const Font TitleFont{ 60, Typeface::Bold };
	const Font StartFont{ 24 };
	const Font ExitFont{ 24 };
	const Audio submit{ U"SE/submit.mp3" };

};


//==============================
// 定数
//==============================
namespace constants {
	namespace brick {
		/// @brief ブロックのサイズ
		constexpr Size SIZE{ 40, 20 };

		/// @brief ブロックの数　縦
		constexpr int Y_COUNT = 5;

		/// @brief ブロックの数　横
		constexpr int X_COUNT = 20;

		/// @brief 合計ブロック数
		constexpr int MAX = Y_COUNT * X_COUNT;
	}

	namespace ball {
		/// @brief ボールの速さ
		constexpr double SPEED = 700.0;
	}

	namespace paddle {
		/// @brief パドルのデフォルトサイズ
		constexpr Size INITIALSIZE{ 60, 10 };
	}

	namespace reflect {
		/// @brief 縦方向ベクトル
		constexpr Vec2 VERTICAL{ 1, -1 };

		/// @brief 横方向ベクトル
		constexpr Vec2 HORIZONTAL{ -1,  1 };
	}

	namespace gameSettings {
		/// @brief 制限時間
		constexpr double TIMELIMIT = 100;
	}

	/// @brief アイテムの降下速度
	constexpr double ITEMSPEED = 200.0;

	constexpr double EVENTINTERVAL = 5.0;
}

//==============================
// クラス宣言
//==============================

	/// @brief ボール
class Ball final {
private:
	/// @brief 速度
	Vec2 velocity;

	/// @brief ボール
	Circle ball;

public:
	/// @brief コンストラクタ
	Ball() : velocity({ 0, -constants::ball::SPEED }), ball({ 400, 400, 8 }) {}

	/// @brief デストラクタ
	~Ball() {}

	/// @brief 更新
	void Update() {
		ball.moveBy(velocity * Scene::DeltaTime());
	}

	/// @brief 描画
	void Draw() const {
		ball.draw(Color(30, 26, 27));
	}

	/// @brief ポインタにアクセスがあった際にインスタンスを返すようのメソッド
	/// @return 
	Circle GetCircle() const {

		return ball;
	}

	/// @brief ポインタにアクセスがあった際に移動速度を返すようのメソッド
	/// @return 
	Vec2 GetVelocity() const {
		return velocity;
	}

	Vec2 GetPosition() const {
		return Vec2(ball.x, ball.y);
	}

	/// @brief 新しい移動速度を設定
	/// @param newVelocity 新しい移動速度
	void SetVelocity(Vec2 newVelocity) {
		using namespace constants::ball;
		velocity = newVelocity.setLength(SPEED);
	}

	/// @brief 反射
	/// @param reflectVec 反射ベクトル方向 
	void Reflect(const Vec2 reflectVec) {
		velocity *= reflectVec;
	}
};

/// @brief ブロック
class Bricks final {
private:
	/// @brief ブロックリスト
	Rect brickTable[constants::brick::MAX];
	const Audio shot{ U"SE/shot.mp3" };

public:
	uint32 remaingBricks = constants::brick::MAX;
	/// @brief コンストラクタ
	Bricks() {
		using namespace constants::brick;
		for (int y = 0; y < Y_COUNT; ++y) {
			for (int x = 0; x < X_COUNT; ++x) {
				int index = y * X_COUNT + x;
				brickTable[index] = Rect{
					x * SIZE.x,
					60 + y * SIZE.y,
					SIZE
				};
			}
		}
	}

	/// @brief デストラクタ
	~Bricks() {}

	/// @brief 衝突検知
	void Intersects(Ball* const target, ScoreManager* currentScore);

	/// @brief 描画
	void Draw() const {
		using namespace constants::brick;

		for (int i = 0; i < MAX; ++i) {
			brickTable[i].stretched(-1).draw(HSV{ 6, 0.87,  0.8 - i * 0.004 });
		}
	}
};

/// @brief パドル
class Paddle final {
private:
	Rect paddle;
	const Audio stretch{ U"SE/stretch.mp3" };

public:
	Size currentSize;
	/// @brief コンストラクタ
	Paddle() : paddle(Rect(Arg::center(Cursor::Pos().x, 500), constants::paddle::INITIALSIZE)) {
		currentSize = constants::paddle::INITIALSIZE;
	}
	/// @brief デストラクタ
	~Paddle() {}

	/// @brief 衝突検知
	void Intersects(Ball* const target) const;

	/// @brief ポインタにアクセスがあった際にインスタンスを返すメソッド
	/// @return 
	Rect GetPaddle() const {
		return paddle;
	}

	/// @brief パドルを大きくするメソッド
	void ExpandSize() {
		paddle.size.x *= 2;
		currentSize.x *= 2;
		stretch.playOneShot();
	}

	/// @brief パドルのサイズをもとに戻す
	void InitSize() {
		paddle.size = constants::paddle::INITIALSIZE;
		currentSize = constants::paddle::INITIALSIZE;
	}

	/// @brief 更新
	void Update() {
		paddle.x = Cursor::Pos().x - (currentSize.x / 2);
	}

	/// @brief 描画
	void Draw() const {
		paddle.rounded(3).draw(Color(157, 21, 36));
	}
};

/// @brief パドルを大きくするアイテム
class StretchItem final {
private:
	/// @brief アイテムの大きさ
	uint8 radius = 30;
	Circle stretchItem;
	/// @brief 速度
	Vec2 velocity;
	bool oldIntersect = false;


public:
	/// @brief コンストラクタ
	/// @param xPos 
	StretchItem(double xPos) :velocity({ 0, constants::ITEMSPEED }), stretchItem(xPos, 0, radius) {};
	~StretchItem() {}

	/// @brief 当たり判定
	/// @param paddle 
	void Intersects(Paddle* const paddle);

	/// @brief 更新 
	void Update() {
		///移動
		stretchItem.moveBy(velocity * Scene::DeltaTime());
	}

	/// @brief 描画
	void Draw() const {
		stretchItem.draw(Color(81, 251, 237));
	}

	/// @brief ポインタにアクセスがあった際に移動速度を返すようのメソッド
/// @return 
	Vec2 GetVelocity() const {
		return velocity;
	}

	/// @brief 新しい移動速度を設定
	/// @param newVelocity 新しい移動速度
	void SetVelocity(Vec2 newVelocity) {
		velocity = newVelocity.setLength(constants::ITEMSPEED);
	}

};

/// @brief アイテムを生成するクラス
class ItemSpawner final {
private:
	/// @brief アイテムを出す場所
	double randomPointX = 0;

public:
	/// @brief アイテムを生成する
	StretchItem* SpawnItem() {
		int randomPointX = 100 * Random(0, 6);
		return new StretchItem(randomPointX);
	}
};

/// @brief 壁
class Wall {
public:
	/// @brief 衝突検知
	static void Intersects(Ball* target) {
		using namespace constants;

		if (!target) {
			return;
		}

		auto velocity = target->GetVelocity();
		auto ball = target->GetCircle();

		// 天井との衝突を検知
		if ((ball.y < 0) && (velocity.y < 0))
		{
			target->Reflect(reflect::VERTICAL);
		}

		// 壁との衝突を検知
		if (((ball.x < 0) && (velocity.x < 0))
			|| ((Scene::Width() < ball.x) && (0 < velocity.x)))
		{
			target->Reflect(reflect::HORIZONTAL);
		}
	}
};



//==============================
// 定義
//==============================
void Bricks::Intersects(Ball* const target, ScoreManager* score) {
	using namespace constants;
	using namespace constants::brick;

	if (!target) {
		return;
	}

	auto ball = target->GetCircle();
	for (int i = 0; i < MAX; ++i) {
		// 参照で保持
		Rect& refBrick = brickTable[i];

		// 衝突を検知
		if (refBrick.intersects(ball))
		{
			// ブロックの上辺、または底辺と交差
			if (refBrick.bottom().intersects(ball)
				|| refBrick.top().intersects(ball))
			{
				target->Reflect(reflect::VERTICAL);
			}
			else // ブロックの左辺または右辺と交差
			{
				target->Reflect(reflect::HORIZONTAL);
			}

			//スコア加算
			score->AddScore(10);
			// あたったブロックは画面外に出す
			refBrick.y -= 600;
			remaingBricks--;
			shot.playOneShot();
			// 同一フレームでは複数のブロック衝突を検知しない
			break;
		}
	}
}

void Paddle::Intersects(Ball* const target) const {
	if (!target) {
		return;
	}

	auto velocity = target->GetVelocity();
	auto ball = target->GetCircle();

	if ((0 < velocity.y) && paddle.intersects(ball))
	{
		target->SetVelocity(Vec2{
			(ball.x - paddle.center().x) * 10,
			-velocity.y
		});
	}
}

void StretchItem::Intersects(Paddle* const target) {
	if (!target) {
		return;
	}

	auto paddle = target->GetPaddle();
	if (!oldIntersect && stretchItem.intersects(paddle))
	{
		target->ExpandSize();
	}
	oldIntersect = stretchItem.intersects(paddle);

}

class Clear : public GameManager::Scene {

public:

	/// @brief コンストラクタ
	Clear(const InitData& init) : IScene{ init } {}

	void update() override
	{
		m_startTransition.update(m_startButton.mouseOver());
		m_exitTransition.update(m_exitButton.mouseOver());

		if (m_startButton.mouseOver() || m_exitButton.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}

		if (m_startButton.leftClicked())
		{
			// タイトルへ
			submit.playOneShot();
			changeScene(State::Title);
		}
		else if (m_exitButton.leftClicked())
		{
			// 終了
			submit.playOneShot();
			System::Exit();
		}
	}

	void draw() const override
	{
		//ボタン描画
		m_startButton.draw(ColorF(U"#51fbed").setA(m_startTransition.value())).drawFrame(1, Color(30, 26, 27));
		m_exitButton.draw(ColorF(U"#51fbed").setA(m_exitTransition.value())).drawFrame(1, Color(30, 26, 27));

		//文字描画
		TextFont(U"Clear!!!!").drawAt(Scene::Center().x, Scene::Center().y - 120, Color(157, 21, 36));
		ScoreFont(U"Score:", ScoreManager::GetScore()).drawAt(Scene::Center().x, Scene::Center().y - 40, Color(157, 21, 36));
		StartFont(U"Back to Title").drawAt(m_startButton.center(), Color(30, 26, 27));
		ExitFont(U"Exit").drawAt(m_exitButton.center(), Color(30, 26, 27));
	}

private:

	/// @brief タイトル・ボタンの描画に関わるメンバ変数
	Rect m_startButton{ Arg::center(Scene::Center().x, Scene::Center().y + 80), 300, 60 };
	Transition m_startTransition{ 0.4s, 0.2s };
	Rect m_exitButton{ Arg::center = Scene::Center().movedBy(0, 200), 300, 60 };
	Transition m_exitTransition{ 0.4s, 0.2s };
	const Font ScoreFont{ 24 };
	const Font TextFont{ 48, Typeface::Bold };
	const Font StartFont{ 24 };
	const Font ExitFont{ 24 };
	const Audio submit{ U"SE/submit.mp3" };
};

class GameOver : public GameManager::Scene {

public:

	/// @brief コンストラクタ
	GameOver(const InitData& init) : IScene{ init } {
	}

	void update() override
	{
		m_startTransition.update(m_startButton.mouseOver());
		m_exitTransition.update(m_exitButton.mouseOver());

		if (m_startButton.mouseOver() || m_exitButton.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}

		if (m_startButton.leftClicked())
		{
			// タイトルへ
			submit.playOneShot();
			changeScene(State::Title);
		}
		else if (m_exitButton.leftClicked())
		{
			// 終了
			submit.playOneShot();
			System::Exit();
		}
	}

	void draw() const override
	{
		//ボタン描画
		m_startButton.draw(ColorF(U"#51fbed").setA(m_startTransition.value())).drawFrame(1, Color(30, 26, 27));
		m_exitButton.draw(ColorF(U"#51fbed").setA(m_exitTransition.value())).drawFrame(1, Color(30, 26, 27));

		//文字描画
		TextFont(U"GameOver...").drawAt(Scene::Center().x, Scene::Center().y - 120, Color(157, 21, 36));
		ScoreFont(U"Score:", ScoreManager::GetScore()).drawAt(Scene::Center().x, Scene::Center().y - 40, Color(157, 21, 36));
		StartFont(U"Back to Title").drawAt(m_startButton.center(), Color(30, 26, 27));
		ExitFont(U"Exit").drawAt(m_exitButton.center(), Color(30, 26, 27));
	}

private:

	/// @brief タイトル・ボタンの描画に関わるメンバ変数
	Rect m_startButton{ Arg::center(Scene::Center().x, Scene::Center().y + 80), 300, 60 };
	Transition m_startTransition{ 0.4s, 0.2s };
	Rect m_exitButton{ Arg::center = Scene::Center().movedBy(0, 200), 300, 60 };
	Transition m_exitTransition{ 0.4s, 0.2s };
	const Font TextFont{ 48, Typeface::Bold };
	const Font ScoreFont{ 24 };
	const Font StartFont{ 24 };
	const Font ExitFont{ 24 };
	const Audio submit{ U"SE/submit.mp3" };

};

class Game : public GameManager::Scene
{

private:
	Bricks bricks;
	Ball ball;
	Paddle paddle;
	ScoreManager score;
	const Font timerText{ 30, Typeface::Bold };
	ItemSpawner spawner;
	Timer timer{ 121s };
	StretchItem* itemPtr = nullptr;
	/// @brief 蓄積時間
	double accumulatedTime = 0.0;
	double x = 0.0;
	const RectF screenRect = Scene::Rect();

	void OnTimer() {
		if (itemPtr == nullptr)	itemPtr = spawner.SpawnItem();
		else {
			delete itemPtr;
			itemPtr = nullptr;
			paddle.InitSize();
		}
	}

	void OnDead() {
		changeScene(State::GameOver);
	}

public:

	Game(const InitData& init) : IScene{ init }
	{
		timer.start();
	}

	void update() override {

		///時間切れでゲームオーバー
		if (timer.reachedZero()) {
			changeScene(State::GameOver);
		}

		if (bricks.remaingBricks <= 0) {
			changeScene(State::Clear);
		}

		///更新
		paddle.Update();
		ball.Update();
		if (itemPtr != nullptr)itemPtr->Update();
		accumulatedTime += Scene::DeltaTime();

		// 蓄積時間が周期を超えたら
		if (constants::EVENTINTERVAL <= accumulatedTime)
		{
			OnTimer();
			accumulatedTime -= constants::EVENTINTERVAL;
		}

		if (!screenRect.intersects(ball.GetCircle())) OnDead();
		///当たり判定
		bricks.Intersects(&ball, &score);
		Wall::Intersects(&ball);
		paddle.Intersects(&ball);
		if (itemPtr != nullptr)itemPtr->Intersects(&paddle);


		///描画
		bricks.Draw();
		ball.Draw();
		paddle.Draw();
		score.Draw();
		if (itemPtr != nullptr)itemPtr->Draw();
		timerText(U"残り時間:", timer).draw(24, 500, 20, Color(30, 26, 27));

	}
};



//==============================
// エントリー
//==============================
void Main()
{
	Scene::SetBackground(Color(242, 233, 231));
	GameManager manager;
	manager.add<Title>(State::Title);
	manager.add<Game>(State::Game);
	manager.add<GameOver>(State::GameOver);
	manager.add<Clear>(State::Clear);

	while (System::Update())
	{
		if (not manager.update()) {
			break;
		}
	}
}
