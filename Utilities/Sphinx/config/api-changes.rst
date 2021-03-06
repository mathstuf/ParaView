API Changes between ParaView versions
=====================================

Changes in 4.2
--------------

Changes to defaults
~~~~~~~~~~~~~~~~~~~
This version includes major overhaul of the ParaView's Python internals towards
one main goal: making the results consistent between ParaView UI and Python
applications e.g. when you create a RenderView, the defaults are setup
consistently irrespective of which interface you are using. Thus, scripts
generated from ParaView's trace which don't describe all property values may
produce different results from previous versions.


Choosing data array for scalar coloring
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
In previous versions, the recommended method for selecting an array to color
with was as follows:

::

    disp = GetDisplayProperties(...)
    disp.ColorArrayName = ("POINT_DATA", "Pressure")
    # OR
    disp.ColorArrayName = ("CELL_DATA", "Pressure")

However, scripts generated from ParaView's trace, would result in the following:

::

    disp.ColorArrayName = "Pressure"
    disp.ColorAttributeType = "POINT_DATA"

The latter is no longer supported i.e. ``ColorAttributeType`` property is no
longer available. You uses ColorArrayName to specify both the array
association and the array name, similar to the API for selecting arrays on
filters. Additionally, the attribute type strings ``POINT_DATA`` and
``CELL_DATA`` while still supported are deprecated. For consistency, you use
``POINTS``, ``CELLS``, etc. instead.

::

   disp.ColorArrayName = ("POINTS", "Pressure")


Chart properties
~~~~~~~~~~~~~~~~
There are three types of changes to APIs that set chart properties.

1. Axis properties were set using arrays that contain elements for all
axes (left, bottom, right and top). Now these settings are separated
such that each axis has its own function. There are three groups of
properties affected.

Color settings used arrays of 12 elements to set the color for all
axes. In the current version we use a function for each axis, each
with 3 elements.

- ``AxisColor``
- ``AxisGridColor``
- ``AxisLabelColor``
- ``AxisTitleColor``

Font properties used arrays of 16 elements, 4 elements for each
axis. In the current version we use a function for each axis and for
each font property. See the also the section on font properties.
a. ``AxisLabelFont``
b. ``AxisTitleFont``

There are various other properties that used arrays of 4 elements, one
element for each axis.

- ``AxisLabelNotation``
- ``AxisLabelPrecision``
- ``AxisLogScale``
- ``AxisTitle``
- ``AxisUseCustomLabels``
- ``AxisUseCustomRange``
- ``ShowAxisGrid``
- ``ShowAxisLabels``

The new function names are obtained by using prefixes Left, Bottom,
Right and Top before the old function names. For example, ``AxisColor``
becomes ``LeftAxisColor``, ``BottomAxisColor``, ``RightAxisColor`` and
``TopAxisColor``.

2. Font properties were set using arrays of 4 elements. The 4 elements
were font family, font size, bold and italic. In the current version we use
a function for each font property. The functions affected are:

- ``ChartTitleFont``
- ``LeftAxisLabelFont``
- ``BottomAxisLabelFont``
- ``RightAxisLabelFont``
- ``TopAxisLabelFont``
- ``LeftAxisTitleFont``
- ``BottomAxisTitleFont``
- ``RightAxisTitleFont``
- ``TopAxisTitleFont``

The new function names can be obtained by replacing Font with FontFamily,
FontSize, Bold and Italic. So ``ChartTitleFont`` becomes
``ChartTitleFontFamily``, ``ChartTitleFontSize``, ``ChartTitleBold``,
``ChartTitleItalic``. Note that function names from bullet b to i are generated
in the previous step.

3. Range properties were set using an array of two elements. In the
current version we use individual functions for the minimum and
maximum element of the range.  Properties affected are:

- ``LeftAxisRange``
- ``BottomAxisRange``
- ``RightAxisRange``
- ``TopAxisRange``

The new function names are obtained by using Minimum and Maximum
suffixes after the old function name. So ``LeftAxisRange`` becomes
``LeftAxisRangeMinimum`` and ``LeftAxisRangeMaximum``.
