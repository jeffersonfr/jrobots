#include "jcanvas/core/japplication.h"
#include "jcanvas/core/jwindow.h"
#include "jcanvas/core/jbufferedimage.h"

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include <random>
#include <ranges>
#include <jcanvas/core/japplication.h>

enum class Action {
  Move,
  Stop,
  Rotate,
  Canon
};

enum class Event {
  HitWall
};

static constexpr int ARENA_LIMIT = 100;

struct Robot;
struct Arena;

struct Images {
  inline static std::shared_ptr<jcanvas::BufferedImage> background{
    new jcanvas::BufferedImage{std::string{"images/background.jpg"}}
  };
  inline static std::shared_ptr<jcanvas::BufferedImage> sprites{
    new jcanvas::BufferedImage{std::string{"images/sprites.png"}}
  };
};

struct Task {
  virtual ~Task() = default;

  virtual bool execute(Robot &robot) {
    return false;
  }

protected:
  explicit Task(Action action): mAction{action} {
  }

private:
  Action mAction;
};

struct Object {
  virtual ~Object() = default;

  // execucao generica dos objetos
  virtual bool execute() = 0;

  virtual bool collide(Object const &) = 0;

  // causa dano em outro objeto
  virtual void damage(Object &);
};

struct Robot {
  friend Arena;

  enum class Move {
    NONE,
    FORWARD,
    BACKWARD
  };

  enum class Turn {
    NONE,
    LEFT,
    RIGHT
  };

  explicit Robot(std::string name): mName{std::move(name)} {
  }

  virtual ~Robot() = default;

  void add_task(std::shared_ptr<Task> task) {
    mTasks.push_back(std::move(task));
  }

  virtual void on_event(Event event) {
    // enum of events (crash, stop, out of ammo)
  }

  bool execute() {
    // parse list, execute a task, when finish execute the next
    if (!mTasks.empty()) {
      std::shared_ptr<Task> task = mTasks.front();

      if (!task->execute(*this)) {
        mTasks.erase(mTasks.begin());
      }
    }

    return true;
  }

  void pos(int x, int y) {
    mX = x;
    mY = y;
  }

  [[nodiscard]] std::pair<int, int> pos() const {
    return {mX, mY};
  }

  [[nodiscard]] int angle() const {
    return mDegrees;
  }

  [[nodiscard]] int canon() const {
    return mCanonDegrees;
  }

  void turn(Turn turn) {
    mTurn = turn;
  }

  [[nodiscard]] Turn turn() const {
    return mTurn;
  }

  void canon_turn(Turn turn) {
    mCanonTurn = turn;
  }

  [[nodiscard]] Turn canon_turn() const {
    return mCanonTurn;
  }

  void move(Move move) {
    mMove = move;
  }

  [[nodiscard]] Move move() const {
    return mMove;
  }

  void stop() {
    mTurn = Turn::NONE;
    mCanonTurn = Turn::NONE;
    mMove = Move::NONE;
  }

  [[nodiscard]] std::string to_string() const {
    std::ostringstream out;

    out << "Robot:[" << mName << "] pos=(" << mX << ", " << mY << "), degrees=" << mDegrees << ", canon=" << mCanonDegrees;

    return out.str();
  }

private:
  std::string mName;
  std::vector<std::shared_ptr<Task> > mTasks;
  int mX{};
  int mY{};
  int mDegrees{};
  int mCanonDegrees{};
  Turn mTurn{};
  Turn mCanonTurn{};
  Move mMove{};

  void angle(int degrees) {
    mDegrees = degrees;
  }

  void canon(int degrees) {
    mCanonDegrees = degrees;
  }
};

struct MoveTask : public Task {
  MoveTask(std::chrono::milliseconds ms, Robot::Move action = Robot::Move::FORWARD)
    : Task{Action::Move}, mAction{action}, mDelay{ms} {
  }

  virtual ~MoveTask() {
  }

  virtual bool execute(Robot &robot) {
    if (mFirst) {
      mFirst = false;

      mTimeout = std::chrono::steady_clock::now() + mDelay;
    }

    if (std::chrono::steady_clock::now() < mTimeout) {
      robot.move(mAction);

      return true;
    }

    robot.move(Robot::Move::NONE);

    return false;
  }

private:
  std::chrono::milliseconds mDelay;
  std::chrono::time_point<std::chrono::steady_clock> mTimeout;
  Robot::Move mAction;
  bool mFirst{true};
};

struct RotateTask : public Task {
  RotateTask(std::chrono::milliseconds ms, Robot::Turn action)
    : Task{Action::Rotate}, mDelay{ms}, mAction{action} {
  }

  ~RotateTask() override = default;

  bool execute(Robot &robot) override {
    if (mFirst) {
      mFirst = false;

      mTimeout = std::chrono::steady_clock::now() + mDelay;
    }

    if (std::chrono::steady_clock::now() < mTimeout) {
      robot.turn(mAction);

      return true;
    }

    robot.turn(Robot::Turn::NONE);

    return false;
  }

private:
  std::chrono::milliseconds mDelay;
  std::chrono::time_point<std::chrono::steady_clock> mTimeout;
  Robot::Turn mAction;
  bool mFirst{true};
};

struct CanonTask : public Task {
  CanonTask(std::chrono::milliseconds ms, Robot::Turn action)
    : Task{Action::Rotate}, mDelay{ms}, mAction{action} {
  }

  ~CanonTask() override = default;

  bool execute(Robot &robot) override {
    if (mFirst) {
      mFirst = false;

      mTimeout = std::chrono::steady_clock::now() + mDelay;
    }

    if (std::chrono::steady_clock::now() < mTimeout) {
      robot.canon_turn(mAction);

      return true;
    }

    robot.canon_turn(Robot::Turn::NONE);

    return false;
  }

private:
  std::chrono::milliseconds mDelay;
  std::chrono::time_point<std::chrono::steady_clock> mTimeout;
  Robot::Turn mAction;
  bool mFirst{true};
};

struct StopTask : public Task {
  StopTask()
    : Task{Action::Stop} {
  }

  virtual ~StopTask() {
  }

  virtual bool execute(Robot &robot) {
    robot.move(Robot::Move::NONE);
    robot.turn(Robot::Turn::NONE);

    return false;
  }

private:
};

struct Arena {
  Arena(jcanvas::jpoint_t<int> size)
    : mSize{size} {
  }

  virtual ~Arena() {
  }

  void add(std::shared_ptr<Robot> robot) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> w(0 + ARENA_LIMIT, mSize.x - 2 * ARENA_LIMIT);
    std::uniform_int_distribution<std::mt19937::result_type> h(0 + ARENA_LIMIT, mSize.y - 2 * ARENA_LIMIT);

    robot->pos(w(rng), h(rng));

    mRobots.push_back(robot);
  }

  bool loop() {
    bool alive = false;
    int walkStep = 10;
    int angleStep = 1;

    for (auto &robot: mRobots) {
      alive = alive | robot->execute();

      float radians = -M_PI_2 + (robot->angle() * std::numbers::pi) / 180.0f;
      jcanvas::jpoint_t<int> dir = jcanvas::jpoint_t<int>{
        static_cast<int>(walkStep * std::cos(radians)),
        static_cast<int>(walkStep * std::sin(radians))
      };

      if (robot->move() == Robot::Move::FORWARD) {
        robot->pos(robot->pos().first + dir.x, robot->pos().second + dir.y);
      } else if (robot->move() == Robot::Move::BACKWARD) {
        robot->pos(robot->pos().first - dir.x, robot->pos().second - dir.y);
      }

      auto [x, y] = robot->pos();

      if (x < ARENA_LIMIT or x > (mSize.x - ARENA_LIMIT) or y < ARENA_LIMIT or y > (mSize.y - ARENA_LIMIT)) {
        if (x < ARENA_LIMIT) {
          x = ARENA_LIMIT;
        }

        if (x > (mSize.x - 2 * ARENA_LIMIT)) {
          x = mSize.x - 2 * ARENA_LIMIT;
        }

        if (y < ARENA_LIMIT) {
          y = ARENA_LIMIT;
        }

        if (y > (mSize.y - 2 * ARENA_LIMIT)) {
          y = mSize.y - 2 * ARENA_LIMIT;
        }

        robot->pos(x, y);
        robot->on_event(Event::HitWall);
      }

      if (robot->turn() == Robot::Turn::LEFT) {
        robot->angle(robot->angle() - angleStep);
      } else if (robot->turn() == Robot::Turn::RIGHT) {
        robot->angle(robot->angle() + angleStep);
      }

      if (robot->canon_turn() == Robot::Turn::LEFT) {
        robot->canon(robot->canon() - angleStep);
      } else if (robot->canon_turn() == Robot::Turn::RIGHT) {
        robot->canon(robot->canon() + angleStep);
      }

      // verificar se bateu em outro carro e envia o evento

      std::cout << robot->to_string() << std::endl;
    }

    return alive;
  }

  [[nodiscard]] jcanvas::jpoint_t<int> get_size() const {
    return mSize;
  }

  [[nodiscard]] std::vector<std::shared_ptr<Robot> > list_robots() const {
    return mRobots;
  }

private:
  std::vector<std::shared_ptr<Robot> > mRobots;
  jcanvas::jpoint_t<int> mSize;
};

struct Ui : public jcanvas::Window {
  Ui(std::unique_ptr<Arena> &&arena)
    : jcanvas::Window(jcanvas::jpoint_t<int>{arena->get_size()}), mArena{std::move(arena)} {
    SetTitle("Arena");
    SetFramesPerSecond(24);

    mCanvas = std::make_shared<jcanvas::BufferedImage>(jcanvas::jpixelformat_t::ARGB, mArena->get_size());
  }

  ~Ui() override = default;

  void Paint(jcanvas::Graphics *g) override {
    jcanvas::Window::Paint(g);

    if (mArena->loop() == false) {
      return;
    }

    jcanvas::Graphics *gcanvas = mCanvas->GetGraphics();

    gcanvas->DrawImage(Images::background, jcanvas::jrect_t<int>{{0, 0}, mCanvas->GetSize()});

    // draw safe area
    jcanvas::jpoint_t<int> safeArea{ARENA_LIMIT, ARENA_LIMIT};

    gcanvas->SetColor(jcanvas::jcolor_name_t::Yellow);
    gcanvas->DrawRectangle(jcanvas::jrect_t<int>{safeArea, mCanvas->GetSize() - 2 * safeArea});

    // tank [0..7]
    std::vector<std::shared_ptr<jcanvas::Image> > tankImages;

    jcanvas::jpoint_t<int> imageSize = Images::sprites->GetSize() / jcanvas::jpoint_t<int>{8, 4};

    for (int i = 0; i < 8; i++) {
      tankImages.push_back(Images::sprites->Crop(jcanvas::jrect_t<int>{{imageSize.x * i, 0}, imageSize}));
    }

    // canon [8]
    auto canonImage = Images::sprites->Crop(jcanvas::jrect_t<int>{{imageSize.x * 0, imageSize.y * 1}, imageSize});

    for (auto const &robot: mArena->list_robots()) {
      auto pos = robot->pos();
      auto radians = static_cast<float>((-robot->angle() * M_PI) / 180.0);
      auto robotRotate = tankImages[(pos.first + pos.second + robot->angle()) % 8]->Rotate(radians);
      auto canonRadians = static_cast<float>((robot->canon() * M_PI) / 180.0);
      auto canonRotate = canonImage->Rotate(radians - canonRadians);

      std::cout << "robot:" << robot->to_string() << std::endl;
      gcanvas->DrawImage(robotRotate, jcanvas::jpoint_t<int>{pos.first, pos.second} - robotRotate->GetSize() / 2);
      gcanvas->DrawImage(canonRotate, jcanvas::jpoint_t<int>{pos.first, pos.second} - canonRotate->GetSize() / 2);
    }

    g->DrawImage(mCanvas, jcanvas::jrect_t<int>{{0, 0}, GetSize()});

    Repaint();
  }

private:
  std::unique_ptr<Arena> mArena;
  std::shared_ptr<jcanvas::BufferedImage> mCanvas;
};

/////////////////////////////////////// robots
struct RabbitRobot : public Robot {
  RabbitRobot()
    : Robot{"Rabbit"} {
    add_task(std::make_shared<MoveTask>(std::chrono::seconds{1})); // verificar se as task podem sair,
    add_task(std::make_shared<RotateTask>(std::chrono::seconds{3}, Robot::Turn::LEFT));
    add_task(std::make_shared<CanonTask>(std::chrono::seconds{3}, Robot::Turn::RIGHT));
    // verificar se as task podem sair,
    add_task(std::make_shared<MoveTask>(std::chrono::seconds{1})); // verificar se as task podem sair,
    add_task(std::make_shared<RotateTask>(std::chrono::seconds{5}, Robot::Turn::LEFT));
    // verificar se as task podem sair,
    add_task(std::make_shared<MoveTask>(std::chrono::seconds{3})); // verificar se as task podem sair,
  }

  virtual void on_event(Event event) {
    // enum of events (crash, stop, out of ammo)
    std::cout << "CRASH ##" << std::endl;
  }
};

struct CrazyRobot : public Robot {
  CrazyRobot()
    : Robot{"Crazy"} {
  }

  virtual void on_event(Event event) {
    // enum of events (crash, stop, out of ammo)
  }
};

/////////////////////////////////////// end

int main() {
  std::unique_ptr<Arena> arena = std::make_unique<Arena>(jcanvas::jpoint_t<int>{1280, 1280});

  arena->add(std::make_shared<RabbitRobot>());
  arena->add(std::make_shared<CrazyRobot>());

  Ui ui{std::move(arena)};

  jcanvas::Application::Loop();

  return 0;
}
