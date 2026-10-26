import geometry;

#ifdef GEOMETRY_INTERNAL_LIMIT
#error "模块接口里的宏泄漏到了导入方"
#endif

int main() {
    return geo::max_sides > 0 ? 0 : 1;
}
