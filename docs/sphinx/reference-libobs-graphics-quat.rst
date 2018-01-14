Quaternion
==========

.. code:: cpp

   #include <graphics/quat.h>

.. type:: struct quat

   Two component quaternion structure.

.. member:: float quat.x

   X component

.. member:: float quat.y

   Y component

.. member:: float quat.z

   Z component

.. member:: float quat.w

   W component

.. member:: float quat.ptr[4]

   Unioned array of all components

---------------------

.. function:: void quat_identity(struct quat *dst)

   Sets a quaternion to {0.0f, 0.0f, 0.0f, 1.0f}.

   :param dst: Destination

---------------------

.. function:: void quat_set(struct quat *dst, float x, float y)

   Sets the individual components of a quaternion.

   :param dst: Destination
   :param x:   X component
   :param y:   Y component
   :param y:   Z component
   :param w:   W component

---------------------

.. function:: void quat_copy(struct quat *dst, const struct quat *v)

   Copies a quaternion

   :param dst: Destination
   :param v:   Quaternion to copy

---------------------

.. function:: void quat_add(struct quat *dst, const struct quat *v1, const struct quat *v2)

   Adds two quaternions

   :param dst: Destination
   :param v1:  Quaternion 1
   :param v2:  Quaternion 2

---------------------

.. function:: void quat_sub(struct quat *dst, const struct quat *v1, const struct quat *v2)

   Subtracts two quaternions

   :param dst: Destination
   :param v1:  Quaternion being subtracted from
   :param v2:  Quaternion being subtracted

---------------------

.. function:: void quat_mul(struct quat *dst, const struct quat *v1, const struct quat *v2)

   Multiplies two quaternions

   :param dst: Destination
   :param v1:  Quaternion 1
   :param v2:  Quaternion 2

---------------------

.. function:: void quat_addf(struct quat *dst, const struct quat *v, float f)

   Adds a floating point to all components

   :param dst: Destination
   :param dst: Quaternion
   :param f:   Floating point

---------------------

.. function:: void quat_subf(struct quat *dst, const struct quat *v, float f)

   Subtracts a floating point from all components

   :param dst: Destination
   :param v:   Quaternion being subtracted from
   :param f:   Floating point being subtracted
   
---------------------

.. function:: void quat_mulf(struct quat *dst, const struct quat *v, float f)

   Multiplies a floating point with all components

   :param dst: Destination
   :param dst: Quaternion
   :param f:   Floating point

---------------------

.. function:: void quat_inv(struct quat *dst, const struct quat *v)

   Inverts a quaternion

   :param dst: Destination
   :param v:   Quaternion to invert

---------------------

.. function:: float quat_dot(const struct quat *v1, const struct quat *v2)

   Performs a dot product between two quaternions

   :param v1: Quaternion 1
   :param v2: Quaternion 2
   :return:   Result of the dot product

---------------------

.. function:: float quat_len(const struct quat *v)

   Gets the length of a quaternion

   :param v: Quaternion
   :return:  The quaternion's length

---------------------

.. function:: float quat_dist(const struct quat *v1, const struct quat *v2)

   Gets the distance between two quaternions

   :param v1: Quaternion 1
   :param v2: Quaternion 2
   :return:   Distance between the two quaternions

---------------------

.. function:: void quat_from_axisang(struct quat *dst, const struct axisang *aa)

   Converts an axis angle to a quaternion

   :param dst: Destination quaternion
   :param aa:  Axis angle

---------------------

.. function:: void quat_from_matrix4(struct quat *dst, const struct matrix4 *m)

   Converts the rotational properties of a matrix to a quaternion

   :param dst: Destination quaternion
   :param m:   Matrix to convert

---------------------

.. function:: void quat_get_dir(struct vec3 *dst, const struct quat *q)

   Converts a quaternion to a directional vector

   :param dst: Destination 3-component vector
   :param q:   Quaternion

---------------------

.. function:: void quat_set_look_dir(struct quat *dst, const struct vec3 *dir)

   Creates a quaternion from a specific "look" direction

   :param dst: Destination quaternion
   :param dir: 3-component vector representing the look direction

---------------------

.. function:: void quat_interpolate(struct quat *dst, const struct quat *q1, const struct quat *q2, float t)

   Linearly interpolates two quaternions

   :param dst: Destination quaternion
   :param q1:  Quaternion 1
   :param q2:  Quaternion 2
   :param t:   Time value (0.0f..1.0f)

---------------------

.. function:: void quat_get_tangent(struct quat *dst, const struct quat *prev, const struct quat *q, const struct quat *next)

   Gets a tangent value for the center of three rotational values

   :param dst:  Destination quaternion
   :param prev: Previous rotation
   :param q:    Rotation to get tangent for
   :param next: Next rotation

---------------------

.. function:: void quat_interpolate_cubic(struct quat *dst, const struct quat *q1, const struct quat *q2, const struct quat *m1, const struct quat *m2, float t)

   Performs cubic interpolation between two quaternions

   :param dst: Destination quaternion
   :param q1:  Quaternion 1
   :param q2:  Quaternion 2
   :param m1:  Tangent 1
   :param m2:  Tangent 2
   :param t:   Time value (0.0f..1.0f)
