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
  HitWall,
  Finish
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

  [[nodiscard]] std::string const &name() const {
    return mName;
  }

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

  void reset() {
    mTasks.clear();

    stop();
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

private:
  std::string mName;
  std::vector<std::shared_ptr<Task> > mTasks;
  Turn mTurn{};
  Turn mCanonTurn{};
  Move mMove{};
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
  struct RobotWrapper {
    explicit RobotWrapper(std::unique_ptr<Robot> &&robot): mRobot{std::move(robot)} {
    }

    virtual ~RobotWrapper() = default;

    std::unique_ptr<Robot> const &robot() {
      return mRobot;
    }

    void pos(int x, int y) {
      mX = x;
      mY = y;
    }

    [[nodiscard]] std::pair<int, int> pos() const {
      return {mX, mY};
    }

    void tank_angle(int degrees) {
      mDegrees = degrees;
    }

    [[nodiscard]] int tank_angle() const {
      return mDegrees;
    }

    void canon_angle(int degrees) {
      mCanonDegrees = degrees;
    }

    [[nodiscard]] int canon_angle() const {
      return mCanonDegrees;
    }

    [[nodiscard]] std::string to_string() const {
      std::ostringstream out;

      out << "Robot:[" << mRobot->name() << "] pos=(" << mX << ", " << mY << "), degrees=" << mDegrees << ", canon=" <<
          mCanonDegrees;

      return out.str();
    }

  private:
    std::unique_ptr<Robot> mRobot;
    int mX{};
    int mY{};
    int mDegrees{};
    int mCanonDegrees{};
  };

  explicit Arena(jcanvas::jpoint_t<int> size)
    : mSize{size} {
  }

  virtual ~Arena() = default;

  void add(std::unique_ptr<Robot> &&robot) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> w(0 + ARENA_LIMIT, mSize.x - 2 * ARENA_LIMIT);
    std::uniform_int_distribution<std::mt19937::result_type> h(0 + ARENA_LIMIT, mSize.y - 2 * ARENA_LIMIT);

    auto wrapper = std::make_unique<RobotWrapper>(std::move(robot));

    wrapper->pos(w(rng), h(rng));

    mRobots.push_back(std::move(wrapper));
  }

  bool loop() {
    bool alive = false;
    int walkStep = 10;
    int angleStep = 1;
    int canonAngleStep = 3;

    for (auto &wrapper: mRobots) {
      bool aliveFlag = wrapper->robot()->execute();

      alive = alive | aliveFlag;

      float radians = -M_PI_2 + (wrapper->tank_angle() * std::numbers::pi) / 180.0f;
      auto dir = jcanvas::jpoint_t<int>{
        static_cast<int>(walkStep * std::cos(radians)),
        static_cast<int>(walkStep * std::sin(radians))
      };

      if (wrapper->robot()->move() == Robot::Move::FORWARD) {
        wrapper->pos(wrapper->pos().first + dir.x, wrapper->pos().second + dir.y);
      } else if (wrapper->robot()->move() == Robot::Move::BACKWARD) {
        wrapper->pos(wrapper->pos().first - dir.x, wrapper->pos().second - dir.y);
      }

      auto [x, y] = wrapper->pos();

      if (x < ARENA_LIMIT or x > (mSize.x - ARENA_LIMIT) or y < ARENA_LIMIT or y > (mSize.y - ARENA_LIMIT)) {
        if (x < ARENA_LIMIT) {
          x = ARENA_LIMIT;
        }

        if (x > (mSize.x - ARENA_LIMIT)) {
          x = mSize.x - ARENA_LIMIT;
        }

        if (y < ARENA_LIMIT) {
          y = ARENA_LIMIT;
        }

        if (y > (mSize.y - ARENA_LIMIT)) {
          y = mSize.y - ARENA_LIMIT;
        }

        wrapper->pos(x, y);
        wrapper->robot()->on_event(Event::HitWall);
      }

      if (aliveFlag == false) {
        wrapper->robot()->on_event(Event::Finish);
      }

      if (wrapper->robot()->turn() == Robot::Turn::LEFT) {
        wrapper->tank_angle(wrapper->tank_angle() - angleStep);
      } else if (wrapper->robot()->turn() == Robot::Turn::RIGHT) {
        wrapper->tank_angle(wrapper->tank_angle() + angleStep);
      }

      if (wrapper->robot()->canon_turn() == Robot::Turn::LEFT) {
        wrapper->canon_angle(wrapper->canon_angle() - canonAngleStep);
      } else if (wrapper->robot()->canon_turn() == Robot::Turn::RIGHT) {
        wrapper->canon_angle(wrapper->canon_angle() + canonAngleStep);
      }

      // verificar se bateu em outro carro e envia o evento

      // std::cout << robot->to_string() << std::endl;
    }

    return alive;
  }

  [[nodiscard]] jcanvas::jpoint_t<int> get_size() const {
    return mSize;
  }

  [[nodiscard]] std::vector<std::unique_ptr<RobotWrapper> > const &list_robots() {
    return mRobots;
  }

private:
  std::vector<std::unique_ptr<RobotWrapper> > mRobots;
  jcanvas::jpoint_t<int> mSize;

  static bool has_collide(std::unique_ptr<RobotWrapper> const &o1, std::unique_ptr<RobotWrapper> const &o2) {
    return false;
  }
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
      auto radians = static_cast<float>((-robot->tank_angle() * M_PI) / 180.0);
      auto robotRotate = tankImages[std::abs(pos.first + pos.second + robot->tank_angle()) % 8]->Rotate(radians);
      auto canonRadians = static_cast<float>((robot->canon_angle() * M_PI) / 180.0);
      auto canonRotate = canonImage->Rotate(radians - canonRadians);

      // std::cout << "robot:" << robot->to_string() << std::endl;
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
    init();
  }

  void init() {
    add_task(std::make_shared<MoveTask>(std::chrono::seconds{1})); // verificar se as task podem sair,
    add_task(std::make_shared<RotateTask>(std::chrono::seconds{3}, Robot::Turn::LEFT));
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

    reset();

    add_task(std::make_shared<RotateTask>(std::chrono::seconds{3}, Robot::Turn::LEFT));

    init();
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

  arena->add(std::make_unique<RabbitRobot>());
  arena->add(std::make_unique<CrazyRobot>());

  Ui ui{std::move(arena)};

  jcanvas::Application::Loop();

  return 0;
}
