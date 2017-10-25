Matrix
======

.. code:: cpp

   #include <graphics/matrix4.h>

.. type:: struct matrix4

   Matrix structure

.. member:: struct vec4 matrix4.x

   X component vector

.. member:: struct vec4 matrix4.y

   Y component vector

.. member:: struct vec4 matrix4.z

   Z component vector

.. member:: struct vec4 matrix4.w

   W component vector

---------------------

.. function:: void matrix4_copy(struct matrix4 *dst, const struct matrix4 *m)

   Copies a matrix

   :param dst: Destination matrix
   :param m:   Matrix to copy

---------------------

.. function:: void matrix4_identity(struct matrix4 *dst)

   Sets an identity matrix

   :param dst: Destination matrix

---------------------

.. function:: void matrix4_from_quat(struct matrix4 *dst, const struct quat *q)

   Converts a quaternion to a matrix

   :param dst: Destination matrix
   :param q:   Quaternion to convert

---------------------

.. function:: void matrix4_from_axisang(struct matrix4 *dst, const struct axisang *aa)

   Converts an axis angle to a matrix

   :param dst: Destination matrix
   :param aa:  Axis angle to convert

---------------------

.. function:: void matrix4_mul(struct matrix4 *dst, const struct matrix4 *m1, const struct matrix4 *m2)

   Multiples two matrices

   :param dst: Destination matrix
   :param m1:  Matrix 1
   :param m2:  Matrix 2

---------------------

.. function:: float matrix4_determinant(const struct matrix4 *m)

   Gets the determinant value of a matrix

   :param m: Matrix
   :return:  Determinant

---------------------

.. function:: void matrix4_translate3v(struct matrix4 *dst, const struct matrix4 *m, const struct vec3 *v)
              void matrix4_translate3f(struct matrix4 *dst, const struct matrix4 *m, float x, float y, float z)

   Translates the matrix by a 3-component vector

   :param dst: Destination matrix
   :param m:   Matrix to translate
   :param v:   Translation vector

---------------------

.. function:: void matrix4_translate4v(struct matrix4 *dst, const struct matrix4 *m, const struct vec4 *v)

   Translates the matrix by a 4-component vector

   :param dst: Destination matrix
   :param m:   Matrix to translate
   :param v:   Translation vector

---------------------

.. function:: void matrix4_rotate(struct matrix4 *dst, const struct matrix4 *m, const struct quat *q)

   Rotates a matrix by a quaternion

   :param dst: Destination matrix
   :param m:   Matrix to rotate
   :param q:   Rotation quaternion

---------------------

.. function:: void matrix4_rotate_aa(struct matrix4 *dst, const struct matrix4 *m, const struct axisang *aa)
              void matrix4_rotate_aa4f(struct matrix4 *dst, const struct matrix4 *m, float x, float y, float z, float rot)

   Rotates a matrix by an axis angle

   :param dst: Destination matrix
   :param m:   Matrix to rotate
   :param aa:  Rotation anxis angle

---------------------

.. function:: void matrix4_scale(struct matrix4 *dst, const struct matrix4 *m, const struct vec3 *v)
              void matrix4_scale3f(struct matrix4 *dst, const struct matrix4 *m, float x, float y, float z)

   Scales each matrix component by the components of a 3-component vector

   :param dst: Destination matrix
   :param m:   Matrix to scale
   :param v:   Scale vector

---------------------

.. function:: bool matrix4_inv(struct matrix4 *dst, const struct matrix4 *m)

   Inverts a matrix

   :param dst: Destination matrix
   :param m:   Matrix to invert

---------------------

.. function:: void matrix4_transpose(struct matrix4 *dst, const struct matrix4 *m)

   Transposes a matrix

   :param dst: Destination matrix
   :param m:   Matrix to transpose
