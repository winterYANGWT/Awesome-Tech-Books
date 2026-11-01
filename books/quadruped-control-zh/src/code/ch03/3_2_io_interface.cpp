// The same controller code on two back ends (section 3.2).
//
// IOInterface declares only send() and recv(); IOROS and IOSDK implement
// them for the simulator and for the real robot.  main() owns the object
// through std::unique_ptr<IOInterface>, switches the back end at run time,
// and the same calls dispatch to a different derived class.  The virtual
// destructor is visible in the output when the pointer is replaced.
//
// Build: clang++ -std=c++17 -O2 -I/usr/include/eigen3 3_2_io_interface.cpp

#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct LowCmd {
  std::array<double, 12> q_des{};
  std::array<double, 12> dq_des{};
  std::array<double, 12> tau_ff{};
  std::array<double, 12> kp{};
  std::array<double, 12> kd{};
};

struct LowState {
  std::array<double, 12> q{};
  std::array<double, 12> dq{};
  std::array<double, 12> tau{};
  std::array<double, 3> rpy{};
  std::array<double, 3> omega_b{};
  std::array<double, 3> acc_b{};
};

class IOInterface {
 public:
  IOInterface() = default;
  virtual ~IOInterface() = default;  // 多态基类必须虚析构

  IOInterface(const IOInterface&) = delete;
  IOInterface& operator=(const IOInterface&) = delete;

  virtual void send(const LowCmd& cmd) = 0;  // 纯虚：只声明，不实现
  virtual void recv(LowState& state) = 0;
};

class IOROS final : public IOInterface {
 public:
  IOROS() { std::cout << "[IOROS] 打开 ROS 2 话题 /joint_cmd、/joint_states\n"; }
  ~IOROS() override { std::cout << "[IOROS] 释放话题与节点\n"; }

  void send(const LowCmd& cmd) override {
    std::cout << "[IOROS::send] 发布 JointCmd, q_des[0]=" << cmd.q_des[0]
              << ", kp[0]=" << cmd.kp[0] << "\n";
  }
  void recv(LowState& state) override {
    // 仿真里由 Gazebo/MuJoCo 填这组数；这里只演示接口形状。
    state.q.fill(0.0);
    state.dq.fill(0.0);
    state.tau.fill(0.0);
    std::cout << "[IOROS::recv] 订阅到 12 个关节状态, q[0]=" << state.q[0]
              << "\n";
  }
};

class IOSDK final : public IOInterface {
 public:
  IOSDK() { std::cout << "[IOSDK] 打开 UDP 套接字, 目标 192.168.123.10\n"; }
  ~IOSDK() override { std::cout << "[IOSDK] 关闭套接字\n"; }

  void send(const LowCmd& cmd) override {
    std::cout << "[IOSDK::send] 打包 LowCmd 报文, q_des[0]=" << cmd.q_des[0]
              << ", kp[0]=" << cmd.kp[0] << "\n";
  }
  void recv(LowState& state) override {
    state.q.fill(0.0);
    state.dq.fill(0.0);
    state.tau.fill(0.0);
    std::cout << "[IOSDK::recv] 解析 LowState 报文, q[0]=" << state.q[0]
              << "\n";
  }
};

// 工厂是唯一知道“现在用哪个后端”的地方；控制器只看见 IOInterface。
std::unique_ptr<IOInterface> make_io(const std::string& backend) {
  if (backend == "sim") {
    return std::make_unique<IOROS>();
  }
  if (backend == "real") {
    return std::make_unique<IOSDK>();
  }
  throw std::invalid_argument("backend must be sim or real");
}

class Controller {
 public:
  explicit Controller(IOInterface* io) : io_(io) {}

  // 换后端时重新绑定；控制器不拥有 IO 对象。
  void set_io(IOInterface* io) { io_ = io; }

  void step(int cycle) {
    LowCmd cmd;
    cmd.q_des[0] = 0.1 * static_cast<double>(cycle);
    cmd.kd.fill(2.0);
    cmd.kp.fill(80.0);
    io_->send(cmd);       // 运行期决定落到 IOROS 还是 IOSDK
    LowState state;
    io_->recv(state);
  }

 private:
  IOInterface* io_;  // 不拥有对象，生命周期由 main 的 unique_ptr 管理
};

int main() {
  std::cout << "后端: sim\n";
  std::unique_ptr<IOInterface> io = make_io("sim");
  Controller controller(io.get());
  for (int i = 0; i < 2; ++i) {
    controller.step(i);
  }

  std::cout << "后端: real（同一个 Controller，换一个对象）\n";
  io = make_io("real");  // 旧对象在这里析构，虚析构函数保证 IOROS 的清理被执行
  controller.set_io(io.get());
  controller.step(2);

  std::cout << "程序结束，unique_ptr 自动释放当前后端\n";
  return 0;
}
