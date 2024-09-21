#include <iostream>
#include <map>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include <random>

enum class Action {
	Move,
	Stop,
	Rotate
};

enum class Event {
	Crash
};

struct Context {
};

struct Robot;

struct Task {
	virtual ~Task() {
	}

	virtual bool execute(Robot &robot) {
		return false;
	}

	protected:
		Task(Action action):
			mAction{action}
		{
		}

	private:
		Action mAction;
};

struct Robot {
	Robot(std::string name):
		mName{name}
	{
	}

	virtual ~Robot() {
	}

	void add_task(std::shared_ptr<Task> task) {
		mTasks.push_back(std::move(task));
	}

	virtual void on_event(Event event) {
		// enum of events (crash, stop, out of ammo)
	}

	bool run(Context &context) {
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

	std::pair<int, int> pos() {
		return {mX, mY};
	}

	void walk(bool param) {
		mWalk = param;

		if (mWalk) {
			mX++;
			mY++;
		}
	}

	void rotate(bool param) {
		mRotate = param;

		if (mRotate) {
			mDegrees++;
		}
	}

	std::string to_string() {
		std::ostringstream out;

		out << "Robot:[" << mName << "] walk=" << mWalk << ", pos=(" << mX << ", " << mY << "), degrees=" << mDegrees;

		return out.str();
	}

	private:
		std::string mName;
		std::vector<std::shared_ptr<Task>> mTasks;
		int mX{};
		int mY{};
		int mDegrees{};
		bool mWalk{};
		bool mRotate{};
};

struct MoveTask : public Task {
	MoveTask(std::chrono::milliseconds ms)
		: Task{Action::Move}, mDelay{ms}
	{
	}

	virtual ~MoveTask() {
	}

	virtual bool execute(Robot &robot) {
		if (mFirst) {
			mFirst = false;

			mTimeout = std::chrono::steady_clock::now() + mDelay;
		}

		if (std::chrono::steady_clock::now() > mTimeout) {
			robot.walk(false);

			return false;
		}

		robot.walk(true);

		return true;
	}

	private:
		std::chrono::milliseconds mDelay;
		std::chrono::time_point<std::chrono::steady_clock> mTimeout;
		bool mFirst{true};
};

struct RotateTask : public Task {
	RotateTask(std::chrono::milliseconds ms)
		: Task{Action::Rotate}, mDelay{ms}
	{
	}

	virtual ~RotateTask() {
	}

	virtual bool execute(Robot &robot) {
		if (mFirst) {
			mFirst = false;

			mTimeout = std::chrono::steady_clock::now() + mDelay;
		}

		if (std::chrono::steady_clock::now() > mTimeout) {
			robot.rotate(false);

			return false;
		}

		robot.rotate(true);

		return true;
	}

	private:
		std::chrono::milliseconds mDelay;
		std::chrono::time_point<std::chrono::steady_clock> mTimeout;
		bool mFirst{true};
};

struct StopTask : public Task {
	StopTask()
		: Task{Action::Stop}
	{
	}

	virtual ~StopTask() {
	}

	virtual bool execute(Robot &robot) {
		robot.walk(false);
		robot.rotate(false);

		return false;
	}

	private:
};

struct RabbitRobot : public Robot {
	RabbitRobot()
		: Robot{"Rabbit"}
	{
		add_task(std::make_shared<MoveTask>(std::chrono::seconds{100})); // verificar se as task podem sair,
		add_task(std::make_shared<RotateTask>(std::chrono::seconds{1})); // verificar se as task podem sair,
		// uma forma seria pegar a task e executar .. na execucao ela poderia ter um decremento 
		// de tempo, por exemplo, que validaria a finalizacao da task (task->is_valid()) apos a execucao
		// da mesma.
		// as task mudariam propriedadas do robo, por exemplo, move setaria a flag walk=1, dessa forma o 
		// eventqueue saberia que eh para posicionar o robo um passo a frente.
	}

	virtual void on_event(Event event) {
		// enum of events (crash, stop, out of ammo)
		std::cout << "CRASH ##" << std::endl;
	}
};

struct CrazyRobot : public Robot {
	CrazyRobot()
		: Robot{"Crazy"}
	{
	}

	virtual void on_event(Event event) {
		// enum of events (crash, stop, out of ammo)
	}
};

template <int WIDTH, int HEIGHT>
struct EventQueue : public Context {

	EventQueue() {
	}

	virtual ~EventQueue() {
	}

	void add(std::shared_ptr<Robot> robot) {
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<std::mt19937::result_type> w(0, WIDTH);
		std::uniform_int_distribution<std::mt19937::result_type> h(0, WIDTH);

		robot->pos(w(rng), h(rng));

		mRobots.push_back(robot);
	}

	void loop() {
		int count = 0;
		bool alive;

		do {
			alive = false;

			for (auto &robot : mRobots) {
				alive = alive | robot->run(*this);

				auto [x, y] = robot->pos();


				if (x < 0 or x > WIDTH or y < 0 or y > HEIGHT) {
					robot->pos(std::min(std::max(0, x), WIDTH), std::min(std::max(0, y), HEIGHT));
					robot->on_event(Event::Crash);
				}

				std::cout << robot->to_string() << std::endl;
			}
			
			count++;

			std::cout << "loop " << count << std::endl;

			std::this_thread::sleep_for(std::chrono::milliseconds{200});
		} while (alive);
	}

	private:
		std::vector<std::shared_ptr<Robot>> mRobots;
};

int main() {
	EventQueue<500, 500> ev;
	
	ev.add(std::make_shared<RabbitRobot>());
	ev.add(std::make_shared<CrazyRobot>());

	ev.loop();
	
	return 0;
}

