// 第 31.4 节：sender 描述工作、receiver 接收结果，start 之前什么都不执行
#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>

struct PrintReceiver {
    void set_value(int v) const {
        std::cout << "receiver: value " << v << "\n";
    }
    void set_error(std::exception_ptr e) const {
        try {
            std::rethrow_exception(e);
        } catch (const std::exception& ex) {
            std::cout << "receiver: error " << ex.what() << "\n";
        }
    }
    void set_stopped() const {
        std::cout << "receiver: stopped\n";
    }
};

struct JustSender {
    int value;
    bool stopped = false;

    template <class Receiver>
    struct Operation {
        int value;
        bool stopped;
        Receiver receiver;

        void start() noexcept {
            std::cout << "  operation starts\n";
            if (stopped) {
                receiver.set_stopped();
            } else {
                receiver.set_value(value);
            }
        }
    };

    template <class Receiver>
    Operation<Receiver> connect(Receiver receiver) const {
        return Operation<Receiver>{value, stopped, std::move(receiver)};
    }
};

template <class F>
struct ThenSender {
    JustSender source;
    F fn;

    template <class Receiver>
    struct Operation {
        JustSender source;
        F fn;
        Receiver receiver;

        struct InnerReceiver {
            F fn;
            Receiver* outer;

            void set_value(int v) const {
                try {
                    outer->set_value(fn(v));
                } catch (...) {
                    outer->set_error(std::current_exception());
                }
            }
            void set_error(std::exception_ptr e) const { outer->set_error(e); }
            void set_stopped() const { outer->set_stopped(); }
        };

        void start() noexcept {
            InnerReceiver inner{fn, &receiver};
            auto op = source.connect(std::move(inner));
            op.start();
        }
    };

    template <class Receiver>
    Operation<Receiver> connect(Receiver receiver) const {
        return Operation<Receiver>{source, fn, std::move(receiver)};
    }
};

template <class F>
ThenSender<F> then(JustSender source, F fn) {
    return ThenSender<F>{source, std::move(fn)};
}

int main() {
    auto pipeline = then(JustSender{20}, [](int v) { return v * 2 + 2; });
    std::cout << "pipeline described; nothing has run\n";

    auto operation = pipeline.connect(PrintReceiver{});
    std::cout << "connected; still nothing has run\n";

    operation.start();
    std::cout << "start returned\n";

    auto failing = then(JustSender{0}, [](int v) {
        if (v == 0) throw std::runtime_error("division by zero");
        return 100 / v;
    });
    auto failed = failing.connect(PrintReceiver{});
    failed.start();

    auto cancelled = then(JustSender{0, true}, [](int v) { return v + 1; });
    auto stopped = cancelled.connect(PrintReceiver{});
    stopped.start();
}
