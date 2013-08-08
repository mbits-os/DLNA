DLNA
====

An UPnP-based LAN radio.

Requirements
------------

Currently the only supported compiler is vc12.0 preview. The project uses ```Boost.Asio``` library. You need trunk version of the Boost and then you must patch the Boost to be able to use vc12 toolset with it.

BOOST
-------------

### Getting
Checkout the newest source code.

    svn co http://svn.boost.org/svn/boost/trunk boost-trunk

### Patching
Patch the code with patches from the VS2013 compability tickets (Source: [[boost] Patch bonanza for VS2013 Preview support](http://lists.boost.org/Archives/boost/2013/07/204953.php)):

<dl>
<dt>Boost.Config VS2013 Preview version bump (https://svn.boost.org/trac/boost/ticket/8753)</dt>
<dd>The Visual Studio 2013 Preview toolchain has _MSC_VER 1800 and _MSC_FULL_VER 180020617. See the attached patch for the minimum changes that are needed to get anywhere at all.</dd>
<dt>Boost.Build needs support for the VS2013 Preview toolset (18.00.20617.1) (https://svn.boost.org/trac/boost/ticket/8754)</dt>
<dd>Visual Studio 2013 Preview is out and claims to be 12.0. I copy-pasted everything related to 11.0 to refer to 12.0 and it Works On My Machine, see attached patch.</dd>
<dt>Boost.Signals VS2013 Preview version bump (https://svn.boost.org/trac/boost/ticket/8755)</dt>
<dd>The BOOST_WORKAROUND macros at boost/signals/detail/named_slot_map.hpp:130 and libs/signals/src/named_slot_map.cpp:27 needs to be increased to <= 1800 in order to encompass the Visual Studio 2013 Preview compiler.</dd>
<dt>Boost.MPL VS2013 Preview version bump (https://svn.boost.org/trac/boost/ticket/8756)</dt>
<dd>The BOOST_WORKAROUND macros at boost/mpl/assert.hpp lines 37 and 247 need to be changed to also consider BOOST_MSVC, == 1800, patch attached.</dd>
<dt>Boost.Serialization lacks algorithm header include for std::min (https://svn.boost.org/trac/boost/ticket/8757)</dt>
<dd>The <algorithm> header providing std::min is not included in boost/archive/iterators/transform_width.hpp, this breaks on Visual Studio 2013 Preview due to library changes.</dd>
<dt>Boost.Asio lacks algorithm header include for std::min (https://svn.boost.org/trac/boost/ticket/8758)</dt>
<dd>The <algorithm> header providing std::min is not included in boost/asio/detail/impl/win_iocp_io_service.hpp, this breaks on Visual Studio 2013 Preview due to library changes.</dd>
<dt>Boost.Asio assumes VisualC++ does not support move operations</dt>
<dd>There is no ticket for this one. The file to patch is <code>boost/asio/detail/config.hpp</code> and the change should be applied on line 100</dd>
<dd><pre>#  if defined(BOOST_MSVC)
#   if BOOST_MSVC >= 1700
#    define BOOST_ASIO_HAS_MOVE 1
#   endif // BOOST_MSVC >= 1700
#  endif // defined(BOOST_MSVC)</pre></dd>
</dl>

### Setting the environment

The **Boost includes/libraries** property file requires ```BOOST_HOME``` environment variable to be set. It should point to the place you have checked out the Boost sources.

### Building

Build the Boost to support vc2

    > cd %BOOST_HOME%
    > bootstrap vc12
    > b2

To get only the minimal number of libraries needed by both ```Boost.Asio``` and DLNA Radio:

    > b2 toolset=msvc-12.0 variant=debug link=static threading=multi runtime-link=static --with-system --with-date_time --with-regex --with-filesystem
    > b2 toolset=msvc-12.0 variant=release link=static threading=multi runtime-link=static --with-system --with-date_time --with-regex --with-filesystem
