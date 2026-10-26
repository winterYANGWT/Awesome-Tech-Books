module catalog;

namespace store {

template <typename T>
T discounted(T price) {
    return price * T{9} / T{10};
}

template int discounted<int>(int);
template double discounted<double>(double);

}  // namespace store
