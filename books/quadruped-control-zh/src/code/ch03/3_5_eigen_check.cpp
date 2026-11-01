// Eigen is the matrix library used by the C++ examples of the original book.
// This check prints the version, builds a rotation matrix from three elementary
// rotations, verifies R^T R = I and det(R) = 1, and solves a 3x3 system.
//
// Build: clang++ -std=c++17 -O2 -I/usr/include/eigen3 3_5_eigen_check.cpp

#include <Eigen/Dense>

#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
  std::cout << "Eigen 版本: " << EIGEN_WORLD_VERSION << "."
            << EIGEN_MAJOR_VERSION << "." << EIGEN_MINOR_VERSION << "\n";

  const double roll = 0.10;
  const double pitch = -0.20;
  const double yaw = 0.30;
  Eigen::Matrix3d R =
      Eigen::AngleAxisd(yaw, Eigen::Vector3d::UnitZ()).toRotationMatrix() *
      Eigen::AngleAxisd(pitch, Eigen::Vector3d::UnitY()).toRotationMatrix() *
      Eigen::AngleAxisd(roll, Eigen::Vector3d::UnitX()).toRotationMatrix();

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "R =\n" << R << "\n";

  const Eigen::Matrix3d should_be_identity = R.transpose() * R;
  const double orthogonality = (should_be_identity - Eigen::Matrix3d::Identity()).norm();
  std::cout << std::scientific << std::setprecision(2);
  std::cout << "||R^T R - I||_F = " << orthogonality << "\n";
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "det(R)          = " << R.determinant() << "\n";

  Eigen::Matrix3d A;
  A << 4.0, 1.0, 2.0,
       1.0, 3.0, 0.0,
       2.0, 0.0, 5.0;
  const Eigen::Vector3d b(1.0, 2.0, 3.0);
  const Eigen::Vector3d x = A.ldlt().solve(b);
  std::cout << "x = [" << x.transpose() << "]\n";
  std::cout << std::scientific << std::setprecision(2);
  std::cout << "||A x - b||_2   = " << (A * x - b).norm() << "\n";
  std::cout << std::fixed << std::setprecision(4);

  const bool ok = orthogonality < 1e-12 &&
                  std::abs(R.determinant() - 1.0) < 1e-12 &&
                  (A * x - b).norm() < 1e-10;
  std::cout << (ok ? "Eigen 检查: 通过" : "Eigen 检查: 失败") << "\n";
  return ok ? 0 : 1;
}
