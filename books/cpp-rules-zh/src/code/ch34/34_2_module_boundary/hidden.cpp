import geometry;

int main() {
    // square 在 detail 里，没有 export，导入方看不到它。
    return static_cast<int>(geo::square(3.0));
}
