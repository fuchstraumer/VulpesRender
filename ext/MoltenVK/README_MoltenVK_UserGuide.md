<a class="site-logo" href="http://moltengl.com/moltenvk/" title="MoltenVK">
	<img src="images/MoltenVK-Logo-Banner.png" alt="MoltenVK Home" style="width:256px;height:auto">
</a>

MoltenVK 0.18.2 User Guide
==========================

Copyright (c) 2014-2017 [The Brenwill Workshop Ltd.](http://www.brenwill.com) All rights reserved.

*This document is written in [Markdown](http://en.wikipedia.org/wiki/Markdown) format. 
For best results, use a Markdown reader. You can also view the 
[formatted document](http://www.moltengl.com/docs/readme/moltenvk-0.18.2-readme-user-guide) online.*



Table of Contents
-----------------

- [**Molten** License Agreement](#license)
- [About **MoltenVK**](#about)
- [Running the **MoltenVK** Demo Applications](#demos)
- [Installing **MoltenVK** in Your *Vulkan* Application](#install)
	- [Install as Static Library Framework](#install_static_lib)
	- [Install as Dynamic Library](#install_dynamic_lib)
- [Interacting with the **MoltenVK** Runtime](#interaction)
- [Activating Your **MoltenVK** License](#licensing)
- [**MoltenVK** Product Support](#support)
- [*Metal* Shaders](#shaders)
- [MoltenShaderConverter Shader Converter Tool](#shader_converter_tool)
- [Troubleshooting Shader Conversion](#spv_vs_msl)
- [Performance Considerations](#performance)
	- [Shader Loading Time](#shader_load_time)
	- [Xcode Configuration](#xcode_config)
	- [Metal System Trace Tool](#trace_tool)
- [Known **MoltenVK** Limitations](#limitations)



<a name="license"></a>

**Molten** License Agreement
-----------------------------

**CAREFULLY READ THE *[Molten LICENSE AGREEMENT](../Molten_LICENSE.md)*, FOUND IN THIS Molten
DISTRIBUTION PACKAGE. BY INSTALLING OR OTHERWISE USING THIS Molten DISTRIBUTION PACKAGE, YOU
ACCEPT AND AGREE TO BE BOUND BY THE TERMS AND CONDITIONS OF THE *Molten LICENSE AGREEMENT*.
IF YOU DO NOT ACCEPT THE TERMS AND CONDITIONS OF THE *Molten LICENSE AGREEMENT*, DO NOT 
INSTALL OR USE THIS Molten DISTRIBUTION PACKAGE.**



<a name="about"></a>

About **MoltenVK**
------------------

**MoltenVK** is an implementation of the [*Vulkan*](https://www.khronos.org/vulkan/) 
graphics API, that runs on Apple's [*Metal*](https://developer.apple.com/metal/) graphics 
framework on both *iOS* and *macOS*.

**MoltenVK** allows you to use the *Vulkan*  graphics API to develop modern, cross-platform, 
high-performance graphical games and applications, and to run them across many platforms, 
including both *iOS* and *macOS*.

*Metal* uses a different shading language, the *Metal Shading Language (MSL)*, than 
*Vulkan*, which uses *SPIR-V*. However, fear not, as **MoltenVK** will automatically convert 
your *SPIR-V* shaders to their *MSL* equivalents. This can be performed transparently at run time, 
using the **Runtime Shader Conversion** feature of **MoltenVK**, or at development time using the 
[**MoltenShaderConverter**](#shader_converter_tool) tool provided with this **Molten** distribution
package.



<a name="demos"></a>

Running the **MoltenVK** Demo Applications
------------------------------------------

You can explore how **MoltenVK** provides *Vulkan* support on *iOS* and *macOS* by investigating
and running the demo applications that come with this **Molten** distribution package. 

The **MoltenVK** demo apps are located in the `MoltenVK/Demos` folder within the **Molten**
distribution package. Each demo app is available as an *Xcode* project.

To review and run all of the available demo apps, open the `Demos/Demos.xcworkspace` workspace 
in *Xcode*.

Please read the [Demos/README_MoltenVK_Demos.md](Demos/README_MoltenVK_Demos.md) document for a 
description of each demo app, and instructions on downloading and running the demo apps. 
Many of the **MoltenVK** demos make use of third-party demo examples, which must be downloaded 
from an external repository. Many of the demo apps allow you to explore a variety of *Vulkan* 
features by modifying *Xcode* build settings. All of this is explained in the 
[README_MoltenVK_Demos.md](Demos/README_MoltenVK_Demos.md) document.


<a name="install"></a>

Installing **MoltenVK** in Your *Vulkan* Application
----------------------------------------------------

<a name="install_static_lib"></a>

###Install as Static Library Framework

Installation of **MoltenVK** is straightforward and easy!

For most applications, you can install **MoltenVK** as a *static library framework* that will be 
embedded directly in your application executable, or a component library within your application.
This is simple and straightforward, and is the recommended installation approach for all applications.

To add **MoltenVK** as a *static library framework* to your existing *Vulkan* application, 
follow the steps in this section. If you're new to **MoltenVK**, it is recommended that you 
start with a smaller project to help you understand the transition, and to reduce the possibility
of needing to make modifications to your [shaders](#shaders) to  ensure their compatibility with 
the *Metal* environment. 

1. Open your application in *Xcode* and select your application's target in the 
   *Project Navigator* panel.

2. Open the *Build Settings* tab, and in the **Framework Search Paths** (aka `FRAMEWORK_SEARCH_PATHS`)
   setting:
    - If building for *iOS*, add an entry that points to the `MoltenVK/iOS` folder, 
      found in the **Molten** distribution package.
    - If building for *macOS*, add an entry that points to the `MoltenVK/macOS` folder, 
      found in the **Molten** distribution package.
   
3. Select the *Build Phases* tab, and open the *Link Binary With Libraries* list.
   - If building for *iOS*, drag the `MoltenVK/iOS/MoltenVK.framework` folder, found 
     in the **Molten** distribution package, to the *Link Binary With Libraries* list.
   - If building for *macOS*, drag the `MoltenVK/macOS/MoltenVK.framework` folder, found 
     in the **Molten** distribution package, to the *Link Binary With Libraries* list.
	
4. In the *Link Binary With Libraries* list, click the **+** button, and (selecting from
   the list of system frameworks) add the `libc++.tbd` framework. If you do not have the
   **Link Frameworks Automatically** (aka `CLANG_MODULES_AUTOLINK`) setting enabled, 
   repeat this step for `Metal.framework` and `QuartzCore.framework` as well.

5. When a *Metal* app is running from *Xcode*, the default ***Scheme*** settings reduce
   performance. To improve performance and gain the benefits of *Metal*, perform the 
   following in *Xcode*:
   
	1. Open the ***Scheme Editor*** for building your main application. You can do 
	   this by selecting ***Edit Scheme...*** from the ***Scheme*** drop-down menu, or select 
	   ***Product -> Scheme -> Edit Scheme...*** from the main menu.
	2. On the ***Info*** tab, set the ***Build Configuration*** to ***Release***, and disable the 
	   ***Debug executable*** check-box.
	3. On the ***Options*** tab, disable both the ***Metal API Validation*** and ***GPU Frame Capture***
	   options. For optimal performance, you may also consider disabling the other simulation
	   and debugging options on this tab. For further information, see the 
	   [Xcode Scheme Settings and Performance](https://developer.apple.com/library/ios/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/Dev-Technique/Dev-Technique.html#//apple_ref/doc/uid/TP40014221-CH8-SW3) 
	   section of Apple's *Metal Programming Guide* documentation.



<a name="install_dynamic_lib"></a>

###Install as Dynamic Library

For some applications, you may prefer to install **MoltenVK** as a dynamic library.
This is only recommended for developers who are used to working with dynamic libraries,
and even then, the preferred approach is to link the **MoltenVK** 
[*static library framework*](#install_static_lib) into a dynamic library of your own 
creation, in order to give you the most flexibility for organizing your dynamic libraries.

In order to install **MoltenVK** as its own dynamic library in your application, 
follow these instructions:

1. Open your application in *Xcode* and select your application's target in the 
   *Project Navigator* panel.

2. On the *Build Settings* tab:
     1. In the **Header Search Paths** (aka `HEADER_SEARCH_PATHS`) setting, add an entry 
        that points to the `MoltenVK/include` folder, found in the **Molten** distribution package.
     2. In the **Library Search Paths** (aka `HEADER_SEARCH_PATHS`) setting, add an entry 
        that points to either the `MoltenVK/iOS` or `MoltenVK/macOS` folder, found in the
        **Molten** distribution package.
     3. In the **Runpath Search Paths** (aka `LD_RUNPATH_SEARCH_PATHS`) setting, add a path 
        that matches the library destination you established in **Step 2** above. If the dynamic 
        library is to be embedded within your application, you would typically set this value to 
        either `@executable_path` or `@loader_path`. The `libMoltenVK.dylib` library is internally 
        configured to be located at `@rpath/libMoltenVK.dylib`.

3. On the *Build Phases* tab, open the *Link Binary With Libraries* list.

   1. Drag the `MoltenVK/iOS/libMoltenVK.dylib` or `MoltenVK/macOS/libMoltenVK.dylib` file, 
      found  in the **Molten** distribution package, to the *Link Binary With Libraries* list.
   2. Click the **+** button, and (selecting from the list of system frameworks) add the
      `libc++.tbd` framework. If you do not have the **Link Frameworks Automatically** 
      (aka `CLANG_MODULES_AUTOLINK`) setting enabled, repeat this step for 
      `Metal.framework` and `QuartzCore.framework` as well.

4. Arrange to install the `libMoltenVK.dylib` file in your application environment:

   - To copy the `libMoltenVK.dylib` file into your application or component library:
        1. On the *Build Phases* tab, add a new *Copy Files* build phase.
        2. Set the *Destination* into which you want to place  the `libMoltenVK.dylib` file.
           Typically this will be *Executables*.
        3. Drag the `MoltenVK/iOS/libMoltenVK.dylib` or `MoltenVK/macOS/libMoltenVK.dylib` 
           file to the *Copy Files* list in this new build phase.
   
   - Alternately, you may create your own installation mechanism to install the 
     `MoltenVK/iOS/libMoltenVK.dylib` or `MoltenVK/macOS/libMoltenVK.dylib` file 
     into a standard *iOS* or *macOS* system library folder on the user's device.


The `Cube-iOS` and `Cube-macOS` **MoltenVK** demo apps, found in the `Demos.xcworkspace`,
located in the `MoltenVK/Demos` folder within the **Molten** distribution package, are simple 
examples of installing *MoltenVK* as a dynamic library embedded within an *iOS* or *macOS* 
application, respectively.



<a name="interaction"></a>

Interacting with the **MoltenVK** Runtime
-----------------------------------------

You programmatically configure and interact with the **MoltenVK** runtime through function 
calls, enumeration values, and capabilities, in exactly the same way you do with other
*Vulkan* implementations. The `MoltenVK.framework` contains several header files that define
access to *Vulkan* and **MoltenVK** function calls.

In your application code, you access *Vulkan* features through the API defined in the standard 
`vulkan.h` header file. This file is included in the **MoltenVK** framework, and can be included 
in your source code files as follows:

	#include <MoltenVK/vulkan/vulkan.h>

If you are linking MoltenVK as a dynamic library 
(see [Install as Dynamic Library](#install_dynamic_lib)), or if you prefer to have
exposed header files, add `MoltenVK/include` to your header search path, and include the 
`vulkan.h` file as follows:

	#include <vulkan/vulkan.h>

In addition to the core *Vulkan* API, **MoltenVK**  also supports the following *Vulkan* extensions:

- `VK_KHR_swapchain`
- `VK_KHR_surface`
- `VK_MVK_ios_surface`
- `VK_MVK_macos_surface`
- `VK_MVK_moltenvk`
- `VK_IMG_format_pvrtc`

In order to visibly display your content on *iOS* or *macOS*, you must enable the
`VK_MVK_ios_surface` or `VK_MVK_macos_surface` extension, and use the functions defined 
for those extensions to create a *Vulkan* rendering surface on *iOS* or *macOS*, respectively.

You can enable each of these extensions by defining the `VK_USE_PLATFORM_IOS_MVK` or 
`VK_USE_PLATFORM_MACOS_MVK` guard macro in your compiler build settings. See the description
of the `mvk_vulkan.h` file below for a convenient way to enable these extensions automatically.

The *Vulkan* API, including the `VK_MVK_ios_surface` and `VK_MVK_macos_surface` surface 
extensions, and other *Vulkan* extensions supported by **MoltenVK** (except `VK_MVK_moltenvk`), 
is described in the 
[*Vulkan 1.0 Spec with MoltenVK Extensions*](Vulkan_1.0_Spec_with_MoltenVK_Extensions.html) 
document.

The `VK_MVK_moltenvk` *Vulkan* extension provides functionality beyond the standard  *Vulkan*
API, to support configuration options, license registration, and behaviour that is specific 
to the **MoltenVK** implementation of *Vulkan*. You can access this functionality by including
the `vk_mvk_moltenvk.h` header file in your code. The `vk_mvk_moltenvk.h` file also includes 
the API documentation for this `VK_MVK_moltenvk` extension.

The following API header files are included in the **MoltenVK** package, each of which 
can be included in your application source code as follows:

	#include <MoltenVK/HEADER_FILE>

where `HEADER_FILE` is one of the following:

- `mvk_vulkan.h` - This is a convenience header file that loads the `vulkan.h` header file
   with the appropriate **MoltenVK** *Vulkan* platform surface extension automatically 
   enabled for *iOS* or *macOS*. Use this header file in place of the `vulkan.h` header file, 
   where access to a **MoltenVK** platform surface extension is required.
   
   - When building for *iOS*, the `mvk_vulkan.h` header file automatically enables the
    `VK_MVK_ios_surface` *Vulkan* extension.
   - When building for *macOS*, the `mvk_vulkan.h` header file automatically enables the
    `VK_MVK_macos_surface` *Vulkan* extension.

- `vk_mvk_moltenvk.h` - Contains declarations and documenation for the functions, structures, 
  and enumerations that define the behaviour of the `VK_MVK_moltenvk` *Vulkan* extension.
  
- `mvkDataTypes.h` - Contains helpful functions for converting between *Vulkan* and *Metal* data types.

- `mlnEnv.h` - Contains foundational environmental configuration, including the version of 
  **MoltenVK**, and build directives that help tell your application how to interact 
  with **MoltenVK** on various platforms. You generally don't use this file directly; it is 
  imported by the automatic other header files.

> **Note:** Previous versions of **MoltenVK** included two further header files, 
  `vk_mvk_ios_surface.h` and `vk_mvk_macos_surface.h`. These header files are 
  no longer available, as the `VK_MVK_ios_surface` and `VK_MVK_macos_surface` extensions
  are now part of `vulkan.h`. Use the `mvk_vulkan.h` header file in place of the
  `vk_mvk_ios_surface.h` and `vk_mvk_macos_surface.h` files. 


<a name="licensing"></a>

Activating Your **MoltenVK** License
------------------------------------

**MoltenVK** is provided under a commercial paid license. You must purchase licenses covering 
the **MoltenVK** features you are using before releasing **MoltenVK** as part of a production 
game or application.

During evaluation, you can run **MoltenVK** without purchasing a license. The same **MoltenVK** 
distribution package can be used for both evaluation and production games and applications, 
and the features and performance are identical in both modes. During evaluation, you will 
see the **MoltenVK** logo displayed as a watermark overlay on your graphics scene. Once valid 
licenses have been purchased and activated to cover the **MoltenVK** features you are using,
this watermark will disappear.

Licenses may be purchased for one or more **MoltenVK** feature sets. Depending on whether you
purchased a single license that covers all the features you are using, or purchased individual
licenses for each features set, you may need to activate one or more licenses within **MoltenVK**.

Each license is composed of two parts, a license *ID* and a license *key*, both of which 
are provided to you when you purchase the license. There are two ways to activate a license
within **MoltenVK**:

1. The preferred method is to enter your license ID and key as compiler build settings in 
   your development environment, and call the `vkActivateMoltenVKLicensesMVK()` function to 
   activate them. If you have multiple licenses, covering multiple **MoltenVK** feature sets,
   you can configure up to four licenses using the following pairs of build settings:

		MLN_LICENSE_ID   and MLN_LICENSE_KEY
		MLN_LICENSE_ID_1 and MLN_LICENSE_KEY_1
		MLN_LICENSE_ID_2 and MLN_LICENSE_KEY_2
		MLN_LICENSE_ID_3 and MLN_LICENSE_KEY_3

   Each element of each pair is a single string defined as a build setting, and should not
   include quote marks. For example, you might configure the following build settings:

		MLN_LICENSE_ID=john.doe@example.com
		MLN_LICENSE_KEY=NOVOT3NGHDZ6OQSCXX4VYNXGI3QLI6Z6

   and if you purchase an additional feature set on a separate license, you can add a 
   second pair of build settings:

		MLN_LICENSE_ID_1=john.doe@example.com
		MLN_LICENSE_KEY_1=MZ4T1Y2LDKBJHAL73JPOEJBHELRHEQJF

   In addition to the license ID and key, for any license activation to take place, you must
   also set the following build setting to indicate that you accept the terms and conditions
   of the *[Molten License Agreement](../Molten_LICENSE.md)*:

		MLN_LICENSE_ACCEPT_TERMS_AND_CONDITIONS=1

   You can call the `vkActivateMoltenVKLicensesMVK()` function at any time, but typically
   you will call it during application startup.

2. If you are unable to use build settings to enter license information, you can call the 
   `vkActivateMoltenVKLicenseMVK(licenseID, licenseKey, acceptLicenseTermsAndConditions)`
   function from within your application, passing a license ID and key directly as a pair
   of null-terminated strings, as well as a boolean affirmation that you accept the terms
   and conditions of the *[Molten License Agreement](../Molten_LICENSE.md)*.

   You can call this function at any time, but typically you will call this function during 
   application startup. You can call this function multiple times to accommodate licenses 
   purchased for multiple individual feature sets. Until a valid license is applied covering 
   each feature set used by your application, **MoltenVK** will operate in evaluation mode.

   Using the `vkActivateMoltenVKLicenseMVK(licenseID, licenseKey, acceptLicenseTermsAndConditions)`
   function is not the preferred method for activating licenses because, in a team environment, 
   it is more difficult to enter valid licenses for each developer from your application code.
   Instead, consider using the `vkActivateMoltenVKLicensesMVK()` function (discussed above), which
   allows you to specify the license information through compiler build settings. Using compiler
   build settings allows you to more easily specify the license information for each developer.

The `vkActivateMoltenVKLicensesMVK()` and `vkActivateMoltenVKLicenseMVK(licenseID, licenseKey,
acceptLicenseTermsAndConditions)` functions are found in the `vk_mvk_moltenvk.h` header file.
Include the following header line to your source code file that calls one of these functions:

	#include <MoltenVK/vk_mvk_moltenvk.h>

   
If your **Molten** license is part of a multi-user pack, you must verify the user count 
with your license purchaser or administrator.

Once you have activated one or more licenses to cover the **MoltenVK** features you are using,
an information message will appear in the console logs for each activated feature set: 

	[mvk-info] Activated MoltenVK Vulkan Core license for 'john.doe@example.com'. Not 'john.doe@example.com'? You can acquire your own license at http://www.moltengl.com.

and the **MoltenVK** logo watermark will no longer be displayed on top of your graphic scene. 
If the watermark remains, ensure that you have indicated acceptance of the terms and conditions
of the *[Molten License Agreement](../Molten_LICENSE.md)*, as described above, and check the
console logs to ensure that your license covers all of the **MoltenVK** features that you are
using, as described in the following sub-section.



<a name="support"></a>

**MoltenVK** Product Support
---------------------------

Support for **MoltenVK** is readily available through the 
[MoltenVK Support Forum](https://moltengl.com/forums/forum/moltenvk-support/). This forum 
is actively and professionally managed by the staff at **MoltenVK**, as well as the 
community of **MoltenVK** users like you. For more advanced support requirements, 
you can [contact us](mailto:sales@moltengl.com) to learn more about professional
services available to help you integrate **MoltenVK** into your application.



<a name="shaders"></a>

*Metal* Shaders
---------------

*Metal* uses a different shader language than *Vulkan*. *Vulkan* uses the new 
*SPIR-V Shading Language (SPIR-V)*, whereas *Metal* uses the *Metal Shading Language (MSL)*.

**MoltenVK** provides several options for creating and running *MSL* versions of your 
existing *SPIR-V* shaders. The following options are presented in order of increasing 
sophistication and difficulty:

- You can use the **Runtime Shader Conversion** feature of **MoltenVK** to automatically and
  transparently convert your *SPIR-V* shaders to *MSL* at runtime, by simply loading your 
  *SPIR-V* shaders as you always have, using the standard *Vulkan* `vkCreateShaderModule()` 
  function. **MoltenVK** will automatically convert the *SPIR-V* code to *MSL* at runtime.
  
- You can use the standard *Vulkan* `vkCreateShaderModule()` function to provide your own *MSL* 
  shader code. To do so, set the value of the *magic number* element of the *SPIR-V* stream to 
  one of the values in the `MVKMSLMagicNumber` enumeration found in the `mvkDataTypes.h` header file. 
  
  The *magic number* element of the *SPIR-V* stream is the first element of the stream, 
  and by setting the value of this element to either `kMVKMagicNumberMSLSourceCode` or
  `kMVKMagicNumberMSLCompiledCode`, on *SPIR-V* code that you submit to the `vkCreateShaderModule()`
  function, you are indicating that the remainder of the *SPIR-V* stream contains either
  *MSL* source code, or *MSL* compiled code, respectively.

  You can use the `MoltenShaderConverter` command-line tool found in this **Molten** distribution 
  package to convert your *SPIR-V* shaders to *MSL* source code, offline at development time,
  in order to create the appropriate *MSL* code to load at runtime. The [section below](#shaders)
  discusses how to use this tool in more detail.

You can mix and match these options in your application. For example, a convenient approach is 
to use **Runtime Shader Conversion** for most *SPIR-V* shaders, and provide pre-converted *MSL*
shader source code for the odd *SPIR-V* shader that proves problematic for runtime conversion.



<a name="shader_converter_tool"></a>

MoltenShaderConverter Shader Converter Tool
-------------------------------------------

The **Molten** distribution package includes the `MoltenShaderConverter` command line tool, 
which allows you to convert your *SPIR-V* shader source code to *MSL* at development time, and 
then supply the *MSL* code to **MoltenVK** using one of the methods described in the 
[*Metal* Shaders](#shaders) section above.

The `MoltenShaderConverter` tool uses the same conversion technology as the **Runtime Shader
Conversion** feature of **MoltenVK**.

The `MoltenShaderConverter` tool has a number of options available from the command line:

- The tool can be used to convert a single *SPIR-V* file to *MSL*, or an entire directory tree 
  of *SPIR-V* files to *MSL*. 

- The tool can be used to convert a single *OpenGL GLSL* file, or an entire directory tree 
  of *GLSL* files to either *SPIR-V* or *MSL*. 

To see a complete list of options, run the `MoltenShaderConverter` tool from the command 
line with no arguments.



<a name="spv_vs_msl"></a>

Troubleshooting Shader Conversion
---------------------------------

The shader converter technology in **MoltenVK** is quite robust, and most *SPIR-V* shaders 
can be converted to *MSL* without any problems. In the case where a conversion issue arises, 
you can address the issue as follows:

- Errors encountered during **Runtime Shader Conversion** are logged to the console.

- To help understand conversion issues during **Runtime Shader Conversion**, you can enable
  the logging of the *SPIR-V* and *MSL* shader source code during conversion as follows:
  
  		#include <MoltenVK/vk_mvk_moltenvk.h>
  		...
  		MVKDeviceConfiguration mvkConfig;
  		vkGetMoltenVKDeviceConfigurationMVK(vkDevice, &mvkConfig);
  		mvkConfig.debugMode = true;
  		vkSetMoltenVKDeviceConfigurationMVK(vkDevice, &mvkConfig);

  Performing these steps will enable debug mode in **MoltenVK**, which includes shader conversion 
  logging, and causes both the incoming *SPIR-V* code and the converted *MSL* source code to be 
  logged to the console (in human-readable form). This allows you to manually verify the conversions, 
  and can help you diagnose issues that might occur during shader conversion.

- For minor issues, you may be able to adjust your *SPIR-V* code so that it behaves the same 
  under *Vulkan*, but is easier to automatically convert to *MSL*.
  
- For more significant issues, you can use the `MoltenShaderConverter` tool to convert the
  shaders at development time, adjust the *MSL* code manually so that it compiles correctly, 
  and use the *MSL* shader code instead of the *SPIR-V* code, using the techniques described
  in the [*Metal* Shaders](#shaders) section above.



<a name="performance"></a>

Performance Considerations
--------------------------

This section discusses various options for improving performance when using **MoltenVK**.


<a name="shader_load_time"></a>

###Shader Loading Time

*Metal* supports pre-compiled shaders, which can improve shader loading and set-up performance,
allowing you to reduce your scene loading time. See the [*Metal* Shaders](#shaders) and
`MoltenShaderConverter` [Shader Converter Tool](#shader_converter_tool) sections above for 
more information about how to use the `MoltenShaderConverter` *Shader Converter Tool* to
create and load pre-compiled *Metal* shaders into **MoltenVK**.


<a name="xcode_config"></a>

###Xcode Configuration

When a *Metal* app is running from *Xcode*, the default ***Scheme*** settings reduce performance. 
Be sure to follow the instructions for configuring your application's ***Scheme*** within *Xcode*,
found in the  in the [installation](#install) section above.


<a name="trace_tool"></a>

###Metal System Trace Tool

To help you get the best performance from your graphics app, in addition to the *GPU Driver* 
template, starting with *Xcode 7* and *iOS 9*, the *Instruments* profiling tool includes the
*Metal System Trace* template. This template can be used to provide detailed tracing of the
CPU and GPU behaviour of your application, allowing you unprecedented performance measurement
and tuning capabilities for apps using *Metal*.



<a name="limitations"></a>

Known **MoltenVK** Limitations
-----------------------------

This section documents the known limitations in this version of **MoltenVK**.

- **MoltenVK** is a Layer-0 driver implementation of *Vulkan*, and currently does not
  support the loading of higher level *Vulkan Layers*.

The following *Vulkan* features have not been implemented in this version of **MoltenVK**:

- Compute pipeline:
	- vkCmdDispatch()
	- vkCmdDispatchIndirect()
	- VK_PIPELINE_BIND_POINT_COMPUTE

- Tessellation and Geometry shader stages.

- Events:
	- vkCreateEvent()
	- vkDestroyEvent()
	- vkGetEventStatus()
	- vkSetEvent()
	- vkResetEvent()
	- vkCmdSetEvent()
	- vkCmdResetEvent()
	- vkCmdWaitEvents()

- Application-controlled memory allocations:
	- VkAllocationCallbacks are ignored
	 
- Sparse memory:
	- vkGetImageSparseMemoryRequirements()
	- vkGetPhysicalDeviceSparseImageFormatProperties()
	- vkQueueBindSparse()
	 
- Pipeline statistics query pool:
	- vkCreateQueryPool(VK_QUERY_TYPE_PIPELINE_STATISTICS)

- VkImageViewCreateInfo::VkComponentMapping supports only the following per-texture swizzles:
	- VK_FORMAT_R8_UNORM: VkComponentMapping.r = VK_COMPONENT_SWIZZLE_R
	- VK_FORMAT_R8G8B8A8_UNORM <->	VK_FORMAT_B8G8R8A8_UNORM
	- VK_FORMAT_R8G8B8A8_SRGB <->	VK_FORMAT_B8G8R8A8_SRGB

---------------

**MoltenVK** and the **Molten Shader Converter** use technology from the open-source [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross) and [SPIR-V-Tools](https://github.com/KhronosGroup/SPIRV-Tools) projects to perform *SPIR-V* 
to *MSL* shader conversion during **Runtime Shader Conversion** and in the `MoltenShaderConverter` tool.

The **Molten Shader Converter** uses technology from the open-source [glslang](https://github.com/KhronosGroup/glslang) 
project to help convert *GLSL* shaders to *SPIR-V* or *MSL* shader code in the `MoltenShaderConverter` tool.
