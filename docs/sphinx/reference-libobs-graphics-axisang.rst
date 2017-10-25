Axis Angle
==========

Provides a helper structure for conversion to quaternions.

.. code:: cpp

   #include <graphics/axisang.h>

.. type:: struct axisang
.. member:: float axisang.x

   X axis

.. member:: float axisang.y

   Y axis

.. member:: float axisang.z

   Z axis

.. member:: float axisang.w

   Angle

.. member:: float axisang.ptr[4]

---------------------

.. function:: void axisang_zero(struct axisang *dst)

   Zeroes the axis angle.

   :param dst: Axis angle

---------------------

.. function:: void axisang_copy(struct axisang *dst, struct axisang *aa)

   Copies an axis angle.

   :param dst: Axis angle to copy to
   :param aa:  Axis angle to copy from

---------------------

.. function:: void axisang_set(struct axisang *dst, float x, float y, float z, float w)

   Sets an axis angle.

   :param dst: Axis angle to set
   :param x:   X axis
   :param y:   Y axis
   :param z:   Z axis
   :param w:   Angle

---------------------

.. function:: void axisang_from_quat(struct axisang *dst, const struct quat *q)

   Creates an axis angle from a quaternion.

   :param dst: Axis angle destination
   :param q:   Quaternion to convert
