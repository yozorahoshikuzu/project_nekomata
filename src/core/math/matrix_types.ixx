export module nekomata2:core.math.matrix_types;
import :core.math.base_matrix;

export namespace nekomata2::math {

using Vector2f   = Matrix<float, 1, 2>;
using Vector3f   = Matrix<float, 1, 3>;
using Vector4f   = Matrix<float, 1, 4>;
using Matrix2x2f = Matrix<float, 2, 2>;
using Matrix2x3f = Matrix<float, 3, 2>;
using Matrix2x4f = Matrix<float, 4, 2>;
using Matrix3x2f = Matrix<float, 2, 3>;
using Matrix3x3f = Matrix<float, 3, 3>;
using Matrix3x4f = Matrix<float, 4, 3>;
using Matrix4x2f = Matrix<float, 2, 4>;
using Matrix4x3f = Matrix<float, 3, 4>;
using Matrix4x4f = Matrix<float, 4, 4>;

using Vector2i   = Matrix<int, 1, 2>;
using Vector3i   = Matrix<int, 1, 3>;
using Vector4i   = Matrix<int, 1, 4>;
using Matrix2x2i = Matrix<int, 2, 2>;
using Matrix2x3i = Matrix<int, 3, 2>;
using Matrix2x4i = Matrix<int, 4, 2>;
using Matrix3x2i = Matrix<int, 2, 3>;
using Matrix3x3i = Matrix<int, 3, 3>;
using Matrix3x4i = Matrix<int, 4, 3>;
using Matrix4x2i = Matrix<int, 2, 4>;
using Matrix4x3i = Matrix<int, 3, 4>;
using Matrix4x4i = Matrix<int, 4, 4>;


}