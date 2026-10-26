// 第 18.3 节：保留图片的结构并共享子树，而不是过早压平成字符值
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class PicBase {
public:
    virtual ~PicBase() = default;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual std::string row(int index) const = 0;
    virtual std::string structure() const = 0;
};

class StringPic : public PicBase {
public:
    explicit StringPic(std::vector<std::string> rows) : rows_(std::move(rows)) {
        for (const auto& line : rows_) {
            width_ = std::max(width_, static_cast<int>(line.size()));
            storedChars_ += static_cast<int>(line.size());
        }
        ++created_;
    }

    int width() const override { return width_; }
    int height() const override { return static_cast<int>(rows_.size()); }

    std::string row(int index) const override {
        if (index < 0 || index >= height()) {
            return std::string(width_, ' ');
        }
        return rows_[index] + std::string(width_ - rows_[index].size(), ' ');
    }

    std::string structure() const override { return "text(" + rows_.front() + ")"; }

    static int created() { return created_; }
    static int storedChars() { return storedChars_; }

private:
    std::vector<std::string> rows_;
    int width_ = 0;
    static inline int created_ = 0;
    static inline int storedChars_ = 0;
};

class FramePic : public PicBase {
public:
    explicit FramePic(std::shared_ptr<const PicBase> child) : child_(std::move(child)) {}

    int width() const override { return child_->width() + 2; }
    int height() const override { return child_->height() + 2; }

    std::string row(int index) const override {
        if (index < 0 || index >= height()) {
            return std::string(width(), ' ');
        }
        if (index == 0 || index == height() - 1) {
            return "+" + std::string(child_->width(), '-') + "+";
        }
        return "|" + child_->row(index - 1) + "|";
    }

    std::string structure() const override { return "frame(" + child_->structure() + ")"; }

private:
    std::shared_ptr<const PicBase> child_;
};

class HCatPic : public PicBase {
public:
    HCatPic(std::shared_ptr<const PicBase> left, std::shared_ptr<const PicBase> right)
        : left_(std::move(left)), right_(std::move(right)) {}

    int width() const override { return left_->width() + right_->width(); }
    int height() const override { return std::max(left_->height(), right_->height()); }

    std::string row(int index) const override {
        if (index < 0 || index >= height()) {
            return std::string(width(), ' ');
        }
        return left_->row(index) + right_->row(index);
    }

    std::string structure() const override {
        return "hcat(" + left_->structure() + ", " + right_->structure() + ")";
    }

private:
    std::shared_ptr<const PicBase> left_;
    std::shared_ptr<const PicBase> right_;
};

class Picture {
public:
    explicit Picture(std::vector<std::string> rows)
        : impl_(std::make_shared<StringPic>(std::move(rows))) {}

    int width() const { return impl_->width(); }
    int height() const { return impl_->height(); }
    std::string structure() const { return impl_->structure(); }

    std::string render() const {
        std::string out;
        for (int i = 0; i < height(); ++i) {
            out += impl_->row(i);
            out += '\n';
        }
        return out;
    }

private:
    explicit Picture(std::shared_ptr<const PicBase> impl) : impl_(std::move(impl)) {}

    friend Picture frame(const Picture& picture);
    friend Picture hcat(const Picture& left, const Picture& right);

    std::shared_ptr<const PicBase> impl_;
};

Picture frame(const Picture& picture) {
    return Picture(std::make_shared<FramePic>(picture.impl_));
}

Picture hcat(const Picture& left, const Picture& right) {
    return Picture(std::make_shared<HCatPic>(left.impl_, right.impl_));
}

int main() {
    Picture text({"hi"});
    Picture framed = frame(text);
    Picture combined = hcat(framed, text);

    std::cout << "structure: " << combined.structure() << '\n';
    std::cout << "size: " << combined.width() << 'x' << combined.height() << '\n';
    std::cout << combined.render();
    std::cout << "StringPic objects: " << StringPic::created() << '\n';
    std::cout << "characters stored in leaves: " << StringPic::storedChars() << '\n';
}
