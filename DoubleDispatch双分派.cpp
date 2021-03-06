#include <iostream>
#include <tuple>
class IValue;
template<class T> struct CValue;
using ValueTypeList = std::tuple<char, unsigned char, signed char, unsigned short, short, unsigned int, int, unsigned long, long, unsigned long long, long long , float, double, long double>;

template<class List> struct GenerateInterface;
template<class First, class...Rest> struct GenerateInterface<std::tuple<First, Rest...>> : public GenerateInterface<std::tuple<Rest...>> {
	virtual std::shared_ptr<IValue> addBy(const First &lhs) const = 0; // 生成一系列addBy(T)接口
	using GenerateInterface<std::tuple<Rest...>>::addBy; // 不用using指令会把基类接口掩盖因为都叫addBy
};
template<> struct GenerateInterface<std::tuple<>> {
	void addBy() = delete; // 因为上面那个using不加点东西没法编译通过，如果这里的addBy不是函数会导致vs2017崩溃
};
class IValue : protected GenerateInterface<ValueTypeList> {
public:
	virtual std::shared_ptr<IValue> add(const IValue &prhs) const = 0; // 给外部调用的接口
};

template<class List, class Final> struct GenerateImplement;
template<class First, class...Rest, class Final> struct GenerateImplement<std::tuple<First, Rest...>, Final> : public GenerateImplement<std::tuple<Rest...>, Final> {
protected:
	std::shared_ptr<IValue> addBy(const First &lhs) const override {
		const auto v = static_cast<const Final &>(*this).value; // 运用CRTP技术直接静态绑定即可
		return std::make_shared<CValue<decltype(lhs + v)>>(lhs + v); // 左右类型都已知（First=T1, Final=CValue<T2>，开始计算结果然后类型擦除返回
	}
};
template<class Final> struct GenerateImplement<std::tuple<>, Final> : public IValue { };
template<class T> struct CValue : public GenerateImplement<ValueTypeList, CValue<T>> {
	explicit CValue(T v) : value(v) {}
	const T value;
	std::shared_ptr<IValue> add(const IValue &prhs) const override { return prhs.addBy(value); } // 已知了左边.value的类型开始访问右边
};

int main()
{
	std::shared_ptr<IValue> a_int = std::make_shared<CValue<int>>(1);
	std::shared_ptr<IValue> b_double = std::make_shared<CValue<unsigned>>(2.5);
	std::shared_ptr<IValue> result = a_int->add(*b_double);
	if(auto real_result = std::dynamic_pointer_cast<CValue<int>>(result))
		std::cout << typeid(*result).name() << std::endl << real_result->value << std::endl;
}