# OASTC

A totally unfinished open-source ASTC texture compression/decompression tool.
You almost certainly don't want to use this.

### Features

* Open source ([MIT license](http://opensource.org/licenses/MIT)), free for all
commercial and non-commercial use.
* Decompression from ASTC to TGA.
* LDR profile support (minus sRGB).
* Test case generator, to compare behaviour against other ASTC decompression
implementations.
* Bit-exact output compared to ARM's ASTC Evaluation Codec, for all the test
cases.

### Missing features

* Compression support.
* sRGB support.
* HDR profile support.
* 3D texture support.
* Support for more useful input and output file formats.
* Performance.
* Portability to non-Linux OSes.

### Alternatives

ARM's [ASTC Evaluation Codec](https://github.com/ARM-software/astc-encoder) is
probably the best tool to use when possible, and comes with source code.
Unfortunately its license is incompatible with the [Open Source
Definition](http://opensource.org/osd), so it cannot be embedded in open source
projects.

Note that ARM's license prohibits the use of their software for developing
hardware. OASTC's copyright license does not prevent that; but ARM probably
holds patents on many parts of ASTC, and probably cares about hardware
implementations. ASTC is royalty-free but there might be some up-front
licensing cost. If you want to develop hardware, talk to ARM or Khronos first.

NVIDIA's [NVASTC](https://developer.nvidia.com/content/astc-compression-gets-cuda-boost)
is a GPU-accelerated encoder.

### How to build

    mkdir build
    cd build
    cmake ..
    make -j8
    make test

### How to run

    ./oastc_dec -i example.astc -o example.tga

### Introduction to ASTC

ASTC is a lossy texture compression algorithm. Its main goals are:

* Industry-wide support, especially in mobile GPUs, so developers can rely on
one format across all the devices their application supports, instead of a
fragmented set of compression formats across different vendors (PVRTC, S3TC,
ETC2, etc).
* Same or better quality than existing formats, at the same bit-rate.
* Much greater flexibility of bit-rate (from 0.89bpp to 8bpp), so developers
can choose the best quality/performance tradeoff for their particular use.
* Support for HDR and 3D textures.

### ASTC hardware support

ASTC defines three profiles:
* LDR Profile - only 2D images, and only LDR colours.
* HDR Profile - only 2D images, but both LDR and HDR colours.
* Full Profile - both 2D and 3D images, and both LDR and HDR colours.

The [Android Extension
Pack](https://developer.android.com/about/versions/android-5.0.html#AndroidExtensionPack)
requires ASTC (LDR profile). This is a recommended set of functionality for
Android Lollipop devices.

OpenGL ES 3.2 includes AEP, so it also requires ASTC (LDR profile).

Most interest in ASTC has come from the mobile space, but Intel's Skylake
processors are the first desktop chips to include ASTC.

Here is a soon-to-be-outdated list of GPU designs currently known to include
ASTC support:

* ARM:
  * [Mali-T622](http://www.arm.com/products/multimedia/mali-cost-efficient-graphics/mali-t622.php),
    [Mali-T624](http://www.arm.com/products/multimedia/mali-performance-efficient-graphics/mali-t624.php),
    [Mali-T628](http://www.arm.com/products/multimedia/mali-performance-efficient-graphics/mali-t628.php),
    [Mali-T720](http://www.arm.com/products/multimedia/mali-cost-efficient-graphics/mali-t720.php),
    [Mali-T760](http://www.arm.com/products/multimedia/mali-performance-efficient-graphics/mali-t760.php),
    [Mali-T860](http://www.arm.com/products/multimedia/mali-performance-efficient-graphics/mali-t860.php),
    [Mali-T880](http://www.arm.com/products/multimedia/mali-performance-efficient-graphics/mali-t880.php) -
      full profile.
  * [Mali-T820](http://www.arm.com/products/multimedia/mali-cost-efficient-graphics/mali-T820.php),
    [Mali-T830](http://www.arm.com/products/multimedia/mali-cost-efficient-graphics/mali-t830.php) -
      "4x4 pixel block size" - sounds like it's not compliant to any standard profile.

* Imagination:
  * [PowerVR Series6XT](http://imgtec.com/powervr/graphics/series6xt/) -
      LDR and HDR, "optional" (some vendors might remove it from their chips).
  * [PowerVR Series7XE](http://imgtec.com/powervr/graphics/series7xe/),
    [PowerVR Series7XT](http://imgtec.com/powervr/graphics/series7xt/) -
      LDR and HDR.

* Qualcomm:
  * [Adreno 4xx Series](https://developer.qualcomm.com/qfile/28557/adreno_opengl_es_developer_guide.pdf) -
      LDR.

* NVIDIA:
  * [Tegra K1](http://www.nvidia.com/content/PDF/tegra_white_papers/Tegra-K1-whitepaper-v1.0.pdf) -
      apparently [LDR](https://gfxbench.com/device.jsp?benchmark=gfx31&os=Android&api=gl&D=NVIDIA+Shield+tablet&testgroup=info).
  * Tegra X1 -
      apparently [LDR](https://gfxbench.com/device.jsp?benchmark=gfx31&D=NVIDIA+Shield+Android+TV&testgroup=info).

* Intel:
  * [Cherry Trail (Gen8 mobile)](https://software.intel.com/sites/default/files/managed/4a/38/Getting-There-First_OpenGLES3.1_Fractal_Combat_X.pdf) -
      LDR.
  * [Skylake (Gen9 desktop)](http://lists.freedesktop.org/archives/mesa-dev/2015-May/084639.html) -
      LDR and HDR.

* Vivante:
  * [GC7000 and others](http://www.vivantecorp.com/index.php/en/media-article/news/258-20140107-vivante-vega-gc7000.html) -
      apparently [LDR](https://gfxbench.com/device.jsp?benchmark=gfx31&os=Android&api=gl&D=Marvell+pxa1928dkb+%28development+board%29&testgroup=info).

### References

* OpenGL ES:
  * [GL ES 3.2 specification](https://www.khronos.org/registry/gles/specs/3.2/es_spec_3.2.pdf#page=528) (LDR profile)
  * [GL_KHR_texture_compression_astc_hdr](https://www.khronos.org/registry/gles/extensions/KHR/texture_compression_astc_hdr.txt) (LDR and HDR profiles)
  * [GL_OES_texture_compression_astc](https://www.khronos.org/registry/gles/extensions/OES/OES_texture_compression_astc.txt) (LDR, HDR, full profiles)
* OpenGL:
  * [GL_KHR_texture_compression_astc_hdr](https://www.opengl.org/registry/specs/KHR/texture_compression_astc_hdr.txt) (LDR and HDR profiles)
* [Adaptive Scalable Texture Compression white paper](http://malideveloper.arm.com/downloads/Stacy_ASTC_white%20paper.pdf), Stacy Smith.
* [Adaptive Scalable Texture Compression paper, High Performance Graphics 2012](http://www.cs.cmu.edu/afs/cs/academic/class/15869-f11/www/readings/nystad12_astc.pdf), Nystad et al.
* [Adaptive Scalable Texture Compression presentation, High Performance Graphics 2012](http://www.highperformancegraphics.net/previous/www_2012/media/Papers/HPG2012_Papers_Nystad.pdf), Nystad et al.

