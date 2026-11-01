// Matrix and vector operations with Eigen (section 4.7).
//
// The program follows the four subsections: declaration/assignment (4.96,
// 4.97), element and block access (4.98, 4.99), common operations, and the
// twelve helper functions of 4.7.4 rebuilt on top of Eigen.  It prints
// operation counts rather than wall-clock times: the book's examples must give
// the same output on every run, and a microsecond reading does not.
//
// expect: M6
// expect: R\^T R
// expect: 解析逆
// expect: 100 组随机

#include <Eigen/Dense>
#include <Eigen/Geometry>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <random>

using Eigen::Matrix3d;
using Eigen::Matrix4d;
using Eigen::MatrixXd;
using Eigen::Vector3d;
using Eigen::Vector4d;

namespace {

std::string sci(double x) {
    std::ostringstream os;
    os << std::scientific << std::setprecision(2) << x;
    return os.str();
}

// ANCHOR: helpers
Matrix3d rotX(double t) {
    Matrix3d R;
    const double c = std::cos(t), s = std::sin(t);
    R << 1, 0, 0,
         0, c, -s,
         0, s, c;
    return R;
}

Matrix3d rotY(double t) {
    Matrix3d R;
    const double c = std::cos(t), s = std::sin(t);
    R << c, 0, s,
         0, 1, 0,
         -s, 0, c;
    return R;
}

Matrix3d rotZ(double t) {
    Matrix3d R;
    const double c = std::cos(t), s = std::sin(t);
    R << c, -s, 0,
         s, c, 0,
         0, 0, 1;
    return R;
}

Matrix3d skew(const Vector3d& v) {
    Matrix3d S;
    S << 0, -v.z(), v.y(),
         v.z(), 0, -v.x(),
         -v.y(), v.x(), 0;
    return S;
}

// R = Rz(yaw) Ry(pitch) Rx(roll), the Y-P-R convention of section 4.6.
Matrix3d rpyToRot(double roll, double pitch, double yaw) {
    return rotZ(yaw) * rotY(pitch) * rotX(roll);
}

Vector3d rotToRpy(const Matrix3d& R) {
    const double sp = std::max(-1.0, std::min(1.0, -R(2, 0)));
    const double pitch = std::asin(sp);
    if (std::abs(std::cos(pitch)) > 1e-9) {
        return Vector3d(std::atan2(R(2, 1), R(2, 2)), pitch,
                        std::atan2(R(1, 0), R(0, 0)));
    }
    return Vector3d(0.0, pitch, std::atan2(-R(0, 1), R(1, 1)));
}

Matrix4d makeT(const Matrix3d& R, const Vector3d& p) {
    Matrix4d T = Matrix4d::Identity();
    T.block<3, 3>(0, 0) = R;
    T.block<3, 1>(0, 3) = p;
    return T;
}

// Analytic inverse: [R^T, -R^T p; 0 1].  Nine multiplications for R^T p.
Matrix4d invT(const Matrix4d& T) {
    Matrix4d out = Matrix4d::Identity();
    const Matrix3d Rt = T.block<3, 3>(0, 0).transpose();
    out.block<3, 3>(0, 0) = Rt;
    out.block<3, 1>(0, 3) = -Rt * T.block<3, 1>(0, 3);
    return out;
}

Vector4d homoVec(const Vector3d& v) {
    Vector4d out;
    out << v, 1.0;
    return out;
}

Vector3d noHomoVec(const Vector4d& v) { return v.head<3>(); }

Matrix3d quatToRot(const Eigen::Quaterniond& q) {
    return q.normalized().toRotationMatrix();
}

Eigen::Quaterniond rotToQuat(const Matrix3d& R) {
    return Eigen::Quaterniond(R).normalized();
}

// 6x6 adjoint for the twist ordering [v; omega]: Ad = [R, [p]x R; 0, R].
Eigen::Matrix<double, 6, 6> adjoint(const Matrix4d& T) {
    Eigen::Matrix<double, 6, 6> Ad = Eigen::Matrix<double, 6, 6>::Zero();
    const Matrix3d R = T.block<3, 3>(0, 0);
    const Vector3d p = T.block<3, 1>(0, 3);
    Ad.block<3, 3>(0, 0) = R;
    Ad.block<3, 3>(0, 3) = skew(p) * R;
    Ad.block<3, 3>(3, 3) = R;
    return Ad;
}
// ANCHOR_END: helpers

}  // namespace

int main() {
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "=== 4.7 Eigen 矩阵运算 ===\n";

    // ANCHOR: matrix
    // 4.96: comma initialisation fills row by row.
    Matrix3d M;
    M << 1, 2, 3,
         4, 5, 6,
         7, 8, 9;
    Vector3d v;
    v << 1, 2, 3;
    std::cout << "\nM (式 4.96):\n" << M << "\nv = " << v.transpose() << "\n";
    std::cout << "元素个数 " << M.size() << "，行 " << M.rows()
              << "，列 " << M.cols() << "\n";

    // 4.97: a vector on the diagonal.
    Matrix3d D = Vector3d(1, 2, 3).asDiagonal();
    std::cout << "\nM = diag(1,2,3) (式 4.97):\n" << D << "\n";

    // Fixed-size matrices are not zero-initialised: setZero first.
    Matrix3d Z;
    Z.setZero();
    std::cout << "setZero 后 Z(0,0)=" << Z(0, 0) << "，Z(2,2)=" << Z(2, 2) << "\n";

    // 4.98: 6x6 block matrix built from an identity and 2*I.
    MatrixXd M6 = MatrixXd::Zero(6, 6);
    M6.block<3, 3>(0, 3).setIdentity();
    M6.block<3, 3>(3, 0) = Vector3d(2, 2, 2).asDiagonal();
    std::cout << "\nM6 (式 4.98):\n" << M6 << "\n";

    // 4.99: segment assignment inside a vector.
    Eigen::VectorXd v6 = Eigen::VectorXd::Zero(6);
    v6.segment<3>(1) = Vector3d(10, 20, 30);
    std::cout << "v6 (式 4.99): " << v6.transpose() << "\n";
    // ANCHOR_END: matrix

    // Assemble an SE(3) from blocks.
    const double a = 30.0 * M_PI / 180.0;
    const Matrix3d R = rotX(a) * rotY(2.0 * a);
    const Vector3d p(0.4, -0.2, 0.7);
    Matrix4d T = makeT(R, p);
    std::cout << "\nT (由 block 拼成):\n" << T << "\n";

    const Matrix3d I3 = Matrix3d::Identity();
    std::cout << "R^T R - I 的最大元素 = " << sci((R.transpose() * R - I3).cwiseAbs().maxCoeff())
              << "\n";
    std::cout << "det R = " << R.determinant() << "\n";
    std::cout << "R^-1 - R^T 的最大元素 = "
              << sci((R.inverse() - R.transpose()).cwiseAbs().maxCoeff()) << "\n";
    std::cout << "R^-1 - Ry(-60) Rx(-30) 的最大元素 = "
              << sci((R.inverse() - rotY(-2.0 * a) * rotX(-a)).cwiseAbs().maxCoeff())
              << "\n";

    // ANCHOR: inverse
    // Analytic inverse versus the general one.
    const Matrix4d Ti = invT(T);
    const Matrix4d Tgen = T.inverse();
    std::cout << "\n解析逆 T^-1:\n" << Ti << "\n";
    std::cout << "T T^-1 - I 的最大元素 = "
              << sci((T * Ti - Matrix4d::Identity()).cwiseAbs().maxCoeff()) << "\n";
    std::cout << "解析逆与通用求逆的最大元素差 = "
              << sci((Ti - Tgen).cwiseAbs().maxCoeff()) << "\n";
    std::cout << "解析逆乘法次数 = 9；4x4 LU 求逆（含 4 次回代）约 100\n";
    // ANCHOR_END: inverse

    // Solve a 3x3 system with a decomposition, not with an explicit inverse.
    Matrix3d A;
    A << 4, -1, 0,
         -1, 4, -1,
         0, -1, 3;
    const Vector3d b(2, 4, 10);
    const Vector3d x = A.ldlt().solve(b);
    std::cout << "\nA x = b 的解 = " << x.transpose() << "\n";
    std::cout << "残差 |Ax-b| = " << sci((A * x - b).norm()) << "\n";
    std::cout << "显式逆解与分解解的差 = "
              << sci((x - A.inverse() * b).norm()) << "\n";

    // Y-P-R round trip through the helper functions.
    const Vector3d rpy(0.3, 0.4, 0.5);
    const Matrix3d R2 = rpyToRot(rpy(0), rpy(1), rpy(2));
    const Vector3d back = rotToRpy(R2);
    std::cout << "\nrpyToRot 再 rotToRpy: 误差 = " << sci((back - rpy).norm()) << "\n";
    std::cout << "skew(v) v 的最大元素 = " << sci((skew(v) * v).cwiseAbs().maxCoeff())
              << "\n";
    const Vector4d hv = homoVec(p);
    std::cout << "homoVec(p) = " << hv.transpose() << "，noHomoVec = "
              << noHomoVec(hv).transpose() << "\n";

    // Quaternion round trip and the adjoint of a homogeneous transform.
    const Eigen::Quaterniond q = rotToQuat(R);
    std::cout << "quatToRot(rotToQuat(R)) - R 的最大元素 = "
              << sci((quatToRot(q) - R).cwiseAbs().maxCoeff()) << "\n";
    Eigen::Matrix<double, 6, 1> xi;
    xi << v, Vector3d(0.2, -0.1, 0.3);
    const Eigen::Matrix<double, 6, 1> Adxi = adjoint(T) * xi;
    const Vector3d v_ref = R * xi.head<3>() + p.cross(R * xi.tail<3>());
    const Vector3d w_ref = R * xi.tail<3>();
    std::cout << "adjoint(T) xi 与 [R v + p x R w; R w] 的最大差 = "
              << sci((Adxi - (Eigen::Matrix<double, 6, 1>() << v_ref, w_ref).finished())
                         .cwiseAbs().maxCoeff())
              << "\n";

    // 100 random transforms: T * T^-1 and analytic inverse.
    std::mt19937 gen(7);
    std::normal_distribution<double> normal(0.0, 1.0);
    std::uniform_real_distribution<double> angle(0.0, M_PI);
    double worst = 0.0, worst_inv = 0.0;
    for (int k = 0; k < 100; ++k) {
        Vector3d axis(normal(gen), normal(gen), normal(gen));
        axis.normalize();
        const Matrix3d Rr = Eigen::AngleAxisd(angle(gen), axis).toRotationMatrix();
        const Vector3d pr(normal(gen), normal(gen), normal(gen));
        const Matrix4d Tr = makeT(Rr, pr);
        const Matrix4d Tri = invT(Tr);
        worst = std::max(worst, (Tr * Tri - Matrix4d::Identity()).cwiseAbs().maxCoeff());
        worst_inv = std::max(worst_inv, (Tri - Tr.inverse()).cwiseAbs().maxCoeff());
    }
    std::cout << "\n100 组随机 T：||T T^-1 - I||_max = " << std::scientific
              << std::setprecision(2) << worst;
    std::cout << "，解析逆与通用逆的最大差 = " << worst_inv << std::defaultfloat
              << std::setprecision(4) << "\n";
    return 0;
}
