# meson setup builddir --buildtype=release --prefix=D:\bento4
# meson install -C builddir

project(
  'bento4',
  'cpp',
  version: '1.6.0-639', # run_command('git', 'describe',  '--tags', '--abbrev=0').stdout().strip()
  license: 'GPL-2.0',
  default_options: [
    'buildtype=debugoptimized',
    'default_library=static'
  ],
  meson_version: '>=0.63.0'
)

ap4_sources = [
  'Source/C++/Codecs/Ap4Ac3Parser.cpp',
  'Source/C++/Codecs/Ap4Ac4Parser.cpp',
  'Source/C++/Codecs/Ap4AdtsParser.cpp',
  'Source/C++/Codecs/Ap4AvcParser.cpp',
  'Source/C++/Codecs/Ap4BitStream.cpp',
  'Source/C++/Codecs/Ap4Eac3Parser.cpp',
  'Source/C++/Codecs/Ap4HevcParser.cpp',
  'Source/C++/Codecs/Ap4Mp4AudioInfo.cpp',
  'Source/C++/Codecs/Ap4NalParser.cpp',
  'Source/C++/Core/Ap4.cpp',
  'Source/C++/Core/Ap48bdlAtom.cpp',
  'Source/C++/Core/Ap4Ac4Utils.cpp',
  'Source/C++/Core/Ap4AinfAtom.cpp',
  'Source/C++/Core/Ap4Atom.cpp',
  'Source/C++/Core/Ap4AtomFactory.cpp',
  'Source/C++/Core/Ap4AtomSampleTable.cpp',
  'Source/C++/Core/Ap4Av1cAtom.cpp',
  'Source/C++/Core/Ap4AvccAtom.cpp',
  'Source/C++/Core/Ap4BlocAtom.cpp',
  'Source/C++/Core/Ap4ByteStream.cpp',
  'Source/C++/Core/Ap4Co64Atom.cpp',
  'Source/C++/Core/Ap4Command.cpp',
  'Source/C++/Core/Ap4CommandFactory.cpp',
  'Source/C++/Core/Ap4CommonEncryption.cpp',
  'Source/C++/Core/Ap4ContainerAtom.cpp',
  'Source/C++/Core/Ap4CttsAtom.cpp',
  'Source/C++/Core/Ap4Dac3Atom.cpp',
  'Source/C++/Core/Ap4Dac4Atom.cpp',
  'Source/C++/Core/Ap4DataBuffer.cpp',
  'Source/C++/Core/Ap4Debug.cpp',
  'Source/C++/Core/Ap4Dec3Atom.cpp',
  'Source/C++/Core/Ap4DecoderConfigDescriptor.cpp',
  'Source/C++/Core/Ap4DecoderSpecificInfoDescriptor.cpp',
  'Source/C++/Core/Ap4Descriptor.cpp',
  'Source/C++/Core/Ap4DescriptorFactory.cpp',
  'Source/C++/Core/Ap4DrefAtom.cpp',
  'Source/C++/Core/Ap4DvccAtom.cpp',
  'Source/C++/Core/Ap4ElstAtom.cpp',
  'Source/C++/Core/Ap4EsDescriptor.cpp',
  'Source/C++/Core/Ap4EsdsAtom.cpp',
  'Source/C++/Core/Ap4Expandable.cpp',
  'Source/C++/Core/Ap4File.cpp',
  'Source/C++/Core/Ap4FileCopier.cpp',
  'Source/C++/Core/Ap4FileWriter.cpp',
  'Source/C++/Core/Ap4FragmentSampleTable.cpp',
  'Source/C++/Core/Ap4FrmaAtom.cpp',
  'Source/C++/Core/Ap4FtypAtom.cpp',
  'Source/C++/Core/Ap4GrpiAtom.cpp',
  'Source/C++/Core/Ap4HdlrAtom.cpp',
  'Source/C++/Core/Ap4HintTrackReader.cpp',
  'Source/C++/Core/Ap4HmhdAtom.cpp',
  'Source/C++/Core/Ap4HvccAtom.cpp',
  'Source/C++/Core/Ap4IkmsAtom.cpp',
  'Source/C++/Core/Ap4IodsAtom.cpp',
  'Source/C++/Core/Ap4Ipmp.cpp',
  'Source/C++/Core/Ap4IproAtom.cpp',
  'Source/C++/Core/Ap4IsfmAtom.cpp',
  'Source/C++/Core/Ap4IsltAtom.cpp',
  'Source/C++/Core/Ap4IsmaCryp.cpp',
  'Source/C++/Core/Ap4LinearReader.cpp',
  'Source/C++/Core/Ap4Marlin.cpp',
  'Source/C++/Core/Ap4MdhdAtom.cpp',
  'Source/C++/Core/Ap4MehdAtom.cpp',
  'Source/C++/Core/Ap4MfhdAtom.cpp',
  'Source/C++/Core/Ap4MfroAtom.cpp',
  'Source/C++/Core/Ap4MoovAtom.cpp',
  'Source/C++/Core/Ap4Movie.cpp',
  'Source/C++/Core/Ap4MovieFragment.cpp',
  'Source/C++/Core/Ap4Mpeg2Ts.cpp',
  'Source/C++/Core/Ap4MvhdAtom.cpp',
  'Source/C++/Core/Ap4NmhdAtom.cpp',
  'Source/C++/Core/Ap4ObjectDescriptor.cpp',
  'Source/C++/Core/Ap4OdafAtom.cpp',
  'Source/C++/Core/Ap4OddaAtom.cpp',
  'Source/C++/Core/Ap4OdheAtom.cpp',
  'Source/C++/Core/Ap4OhdrAtom.cpp',
  'Source/C++/Core/Ap4OmaDcf.cpp',
  'Source/C++/Core/Ap4PdinAtom.cpp',
  'Source/C++/Core/Ap4Piff.cpp',
  'Source/C++/Core/Ap4Processor.cpp',
  'Source/C++/Core/Ap4Protection.cpp',
  'Source/C++/Core/Ap4PsshAtom.cpp',
  'Source/C++/Core/Ap4Results.cpp',
  'Source/C++/Core/Ap4RtpAtom.cpp',
  'Source/C++/Core/Ap4RtpHint.cpp',
  'Source/C++/Core/Ap4SaioAtom.cpp',
  'Source/C++/Core/Ap4SaizAtom.cpp',
  'Source/C++/Core/Ap4Sample.cpp',
  'Source/C++/Core/Ap4SampleDescription.cpp',
  'Source/C++/Core/Ap4SampleEntry.cpp',
  'Source/C++/Core/Ap4SampleSource.cpp',
  'Source/C++/Core/Ap4SampleTable.cpp',
  'Source/C++/Core/Ap4SbgpAtom.cpp',
  'Source/C++/Core/Ap4SchmAtom.cpp',
  'Source/C++/Core/Ap4SdpAtom.cpp',
  'Source/C++/Core/Ap4SegmentBuilder.cpp',
  'Source/C++/Core/Ap4SencAtom.cpp',
  'Source/C++/Core/Ap4SgpdAtom.cpp',
  'Source/C++/Core/Ap4SidxAtom.cpp',
  'Source/C++/Core/Ap4SLConfigDescriptor.cpp',
  'Source/C++/Core/Ap4SmhdAtom.cpp',
  'Source/C++/Core/Ap4StcoAtom.cpp',
  'Source/C++/Core/Ap4SthdAtom.cpp',
  'Source/C++/Core/Ap4String.cpp',
  'Source/C++/Core/Ap4StscAtom.cpp',
  'Source/C++/Core/Ap4StsdAtom.cpp',
  'Source/C++/Core/Ap4StssAtom.cpp',
  'Source/C++/Core/Ap4StszAtom.cpp',
  'Source/C++/Core/Ap4SttsAtom.cpp',
  'Source/C++/Core/Ap4Stz2Atom.cpp',
  'Source/C++/Core/Ap4SyntheticSampleTable.cpp',
  'Source/C++/Core/Ap4TencAtom.cpp',
  'Source/C++/Core/Ap4TfdtAtom.cpp',
  'Source/C++/Core/Ap4TfhdAtom.cpp',
  'Source/C++/Core/Ap4TfraAtom.cpp',
  'Source/C++/Core/Ap4TimsAtom.cpp',
  'Source/C++/Core/Ap4TkhdAtom.cpp',
  'Source/C++/Core/Ap4Track.cpp',
  'Source/C++/Core/Ap4TrakAtom.cpp',
  'Source/C++/Core/Ap4TrefTypeAtom.cpp',
  'Source/C++/Core/Ap4TrexAtom.cpp',
  'Source/C++/Core/Ap4TrunAtom.cpp',
  'Source/C++/Core/Ap4UrlAtom.cpp',
  'Source/C++/Core/Ap4Utils.cpp',
  'Source/C++/Core/Ap4UuidAtom.cpp',
  'Source/C++/Core/Ap4VmhdAtom.cpp',
  'Source/C++/Core/Ap4VpccAtom.cpp',
  'Source/C++/Crypto/Ap4AesBlockCipher.cpp',
  'Source/C++/Crypto/Ap4Hmac.cpp',
  'Source/C++/Crypto/Ap4KeyWrap.cpp',
  'Source/C++/Crypto/Ap4StreamCipher.cpp',
  'Source/C++/MetaData/Ap4MetaData.cpp',
  'Source/C++/System/StdC/Ap4StdCFileByteStream.cpp'
]

if host_machine.system() == 'windows'
  ap4_sources += 'Source/C++/System/Win32/Ap4Win32Random.cpp'
else
  ap4_sources += 'Source/C++/System/Posix/Ap4PosixRandom.cpp'
endif

ap4_headers = [
  'Source/C++/Codecs/Ap4Ac3Parser.h',
  'Source/C++/Codecs/Ap4Ac4Parser.h',
  'Source/C++/Codecs/Ap4AdtsParser.h',
  'Source/C++/Codecs/Ap4AvcParser.h',
  'Source/C++/Codecs/Ap4BitStream.h',
  'Source/C++/Codecs/Ap4Eac3Parser.h',
  'Source/C++/Codecs/Ap4HevcParser.h',
  'Source/C++/Codecs/Ap4Mp4AudioInfo.h',
  'Source/C++/Codecs/Ap4NalParser.h',
  'Source/C++/Core/Ap4.h',
  'Source/C++/Core/Ap48bdlAtom.h',
  'Source/C++/Core/Ap4Ac4Utils.h',
  'Source/C++/Core/Ap4AinfAtom.h',
  'Source/C++/Core/Ap4Array.h',
  'Source/C++/Core/Ap4Atom.h',
  'Source/C++/Core/Ap4AtomFactory.h',
  'Source/C++/Core/Ap4AtomSampleTable.h',
  'Source/C++/Core/Ap4Av1cAtom.h',
  'Source/C++/Core/Ap4AvccAtom.h',
  'Source/C++/Core/Ap4BlocAtom.h',
  'Source/C++/Core/Ap4ByteStream.h',
  'Source/C++/Core/Ap4Co64Atom.h',
  'Source/C++/Core/Ap4Command.h',
  'Source/C++/Core/Ap4CommandFactory.h',
  'Source/C++/Core/Ap4CommonEncryption.h',
  'Source/C++/Core/Ap4Config.h',
  'Source/C++/Core/Ap4Constants.h',
  'Source/C++/Core/Ap4ContainerAtom.h',
  'Source/C++/Core/Ap4CttsAtom.h',
  'Source/C++/Core/Ap4Dac3Atom.h',
  'Source/C++/Core/Ap4Dac4Atom.h',
  'Source/C++/Core/Ap4DataBuffer.h',
  'Source/C++/Core/Ap4Debug.h',
  'Source/C++/Core/Ap4Dec3Atom.h',
  'Source/C++/Core/Ap4DecoderConfigDescriptor.h',
  'Source/C++/Core/Ap4DecoderSpecificInfoDescriptor.h',
  'Source/C++/Core/Ap4Descriptor.h',
  'Source/C++/Core/Ap4DescriptorFactory.h',
  'Source/C++/Core/Ap4DrefAtom.h',
  'Source/C++/Core/Ap4DvccAtom.h',
  'Source/C++/Core/Ap4DynamicCast.h',
  'Source/C++/Core/Ap4ElstAtom.h',
  'Source/C++/Core/Ap4EsDescriptor.h',
  'Source/C++/Core/Ap4EsdsAtom.h',
  'Source/C++/Core/Ap4Expandable.h',
  'Source/C++/Core/Ap4File.h',
  'Source/C++/Core/Ap4FileByteStream.h',
  'Source/C++/Core/Ap4FileCopier.h',
  'Source/C++/Core/Ap4FileWriter.h',
  'Source/C++/Core/Ap4FragmentSampleTable.h',
  'Source/C++/Core/Ap4FrmaAtom.h',
  'Source/C++/Core/Ap4FtypAtom.h',
  'Source/C++/Core/Ap4GrpiAtom.h',
  'Source/C++/Core/Ap4HdlrAtom.h',
  'Source/C++/Core/Ap4HintTrackReader.h',
  'Source/C++/Core/Ap4HmhdAtom.h',
  'Source/C++/Core/Ap4HvccAtom.h',
  'Source/C++/Core/Ap4IkmsAtom.h',
  'Source/C++/Core/Ap4Interfaces.h',
  'Source/C++/Core/Ap4IodsAtom.h',
  'Source/C++/Core/Ap4Ipmp.h',
  'Source/C++/Core/Ap4IproAtom.h',
  'Source/C++/Core/Ap4IsfmAtom.h',
  'Source/C++/Core/Ap4IsltAtom.h',
  'Source/C++/Core/Ap4IsmaCryp.h',
  'Source/C++/Core/Ap4LinearReader.h',
  'Source/C++/Core/Ap4List.h',
  'Source/C++/Core/Ap4Marlin.h',
  'Source/C++/Core/Ap4MdhdAtom.h',
  'Source/C++/Core/Ap4MehdAtom.h',
  'Source/C++/Core/Ap4MfhdAtom.h',
  'Source/C++/Core/Ap4MfroAtom.h',
  'Source/C++/Core/Ap4MoovAtom.h',
  'Source/C++/Core/Ap4Movie.h',
  'Source/C++/Core/Ap4MovieFragment.h',
  'Source/C++/Core/Ap4Mpeg2Ts.h',
  'Source/C++/Core/Ap4MvhdAtom.h',
  'Source/C++/Core/Ap4NmhdAtom.h',
  'Source/C++/Core/Ap4ObjectDescriptor.h',
  'Source/C++/Core/Ap4OdafAtom.h',
  'Source/C++/Core/Ap4OddaAtom.h',
  'Source/C++/Core/Ap4OdheAtom.h',
  'Source/C++/Core/Ap4OhdrAtom.h',
  'Source/C++/Core/Ap4OmaDcf.h',
  'Source/C++/Core/Ap4PdinAtom.h',
  'Source/C++/Core/Ap4Piff.h',
  'Source/C++/Core/Ap4Processor.h',
  'Source/C++/Core/Ap4Protection.h',
  'Source/C++/Core/Ap4PsshAtom.h',
  'Source/C++/Core/Ap4Results.h',
  'Source/C++/Core/Ap4RtpAtom.h',
  'Source/C++/Core/Ap4RtpHint.h',
  'Source/C++/Core/Ap4SaioAtom.h',
  'Source/C++/Core/Ap4SaizAtom.h',
  'Source/C++/Core/Ap4Sample.h',
  'Source/C++/Core/Ap4SampleDescription.h',
  'Source/C++/Core/Ap4SampleEntry.h',
  'Source/C++/Core/Ap4SampleSource.h',
  'Source/C++/Core/Ap4SampleTable.h',
  'Source/C++/Core/Ap4SbgpAtom.h',
  'Source/C++/Core/Ap4SchmAtom.h',
  'Source/C++/Core/Ap4SdpAtom.h',
  'Source/C++/Core/Ap4SegmentBuilder.h',
  'Source/C++/Core/Ap4SencAtom.h',
  'Source/C++/Core/Ap4SgpdAtom.h',
  'Source/C++/Core/Ap4SidxAtom.h',
  'Source/C++/Core/Ap4SLConfigDescriptor.h',
  'Source/C++/Core/Ap4SmhdAtom.h',
  'Source/C++/Core/Ap4StcoAtom.h',
  'Source/C++/Core/Ap4SthdAtom.h',
  'Source/C++/Core/Ap4String.h',
  'Source/C++/Core/Ap4StscAtom.h',
  'Source/C++/Core/Ap4StsdAtom.h',
  'Source/C++/Core/Ap4StssAtom.h',
  'Source/C++/Core/Ap4StszAtom.h',
  'Source/C++/Core/Ap4SttsAtom.h',
  'Source/C++/Core/Ap4Stz2Atom.h',
  'Source/C++/Core/Ap4SyntheticSampleTable.h',
  'Source/C++/Core/Ap4TencAtom.h',
  'Source/C++/Core/Ap4TfdtAtom.h',
  'Source/C++/Core/Ap4TfhdAtom.h',
  'Source/C++/Core/Ap4TfraAtom.h',
  'Source/C++/Core/Ap4TimsAtom.h',
  'Source/C++/Core/Ap4TkhdAtom.h',
  'Source/C++/Core/Ap4Track.h',
  'Source/C++/Core/Ap4TrakAtom.h',
  'Source/C++/Core/Ap4TrefTypeAtom.h',
  'Source/C++/Core/Ap4TrexAtom.h',
  'Source/C++/Core/Ap4TrunAtom.h',
  'Source/C++/Core/Ap4Types.h',
  'Source/C++/Core/Ap4UrlAtom.h',
  'Source/C++/Core/Ap4Utils.h',
  'Source/C++/Core/Ap4UuidAtom.h',
  'Source/C++/Core/Ap4Version.h',
  'Source/C++/Core/Ap4VmhdAtom.h',
  'Source/C++/Core/Ap4VpccAtom.h',
  'Source/C++/Crypto/Ap4AesBlockCipher.h',
  'Source/C++/Crypto/Ap4Hmac.h',
  'Source/C++/Crypto/Ap4KeyWrap.h',
  'Source/C++/Crypto/Ap4StreamCipher.h',
  'Source/C++/Metadata/Ap4MetaData.h'
]

ap4_include = [
  include_directories('Source/C++/Codecs'),
  include_directories('Source/C++/Core'),
  include_directories('Source/C++/Crypto'),
  include_directories('Source/C++/MetaData')
]

if get_option('default_library') == 'both'
  error('default library cannot be set to both, it can be either static or shared')
endif

if meson.get_compiler('cpp').get_id() == 'msvc' and get_option('default_library') == 'shared'
  warning('building shared library with msvc doesn\'t export any symbols')
endif

ap4_lib = library(
  'ap4',
  ap4_sources,
  install: true,
  include_directories: ap4_include,
)

project_dep = declare_dependency(
  include_directories: ap4_include,
  link_with: ap4_lib
)
set_variable(meson.project_name() + '_dep', project_dep)

install_headers(
  ap4_headers + [
    'Source/C++/Adapters/Ap4AtomixAdapters.h',
    'Source/C++/Adapters/Ap4NeptuneAdapters.h',
    'Source/C++/CApi/Bento4C.h'
  ]
)

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name: meson.project_name(),
  filebase: meson.project_name(),
  description: 'Full-featured MP4 format, MPEG DASH, HLS, CMAF SDK and tools',
  libraries: ap4_lib,
)

if get_option('build-apps')
  test_apps = get_option('test-apps')
  apps = [
    ['aac2mp4', 'Source/C++/Apps/Aac2Mp4/Aac2Mp4.cpp'],
    ['avcinfo', 'Source/C++/Apps/AvcInfo/AvcInfo.cpp'],
    ['fixaacsampledescription', 'Source/C++/Apps/FixAacSampleDescription/FixAacSampleDescription.cpp'],
    ['hevcinfo', 'Source/C++/Apps/HevcInfo/HevcInfo.cpp'],
    ['mp42aac', 'Source/C++/Apps/Mp42Aac/Mp42Aac.cpp'],
    ['mp42avc', 'Source/C++/Apps/Mp42Avc/Mp42Avc.cpp'],
    ['mp42hevc', 'Source/C++/Apps/Mp42Hevc/Mp42Hevc.cpp'],
    ['mp42hls', 'Source/C++/Apps/Mp42Hls/Mp42Hls.cpp'],
    ['mp42ts', 'Source/C++/Apps/Mp42Ts/Mp42Ts.cpp'],
    ['mp4audioclip', 'Source/C++/Apps/Mp4AudioClip/Mp4AudioClip.cpp'],
    ['mp4compact', 'Source/C++/Apps/Mp4Compact/Mp4Compact.cpp'],
    ['mp4dcfpackager', 'Source/C++/Apps/Mp4DcfPackager/Mp4DcfPackager.cpp'],
    ['mp4decrypt', 'Source/C++/Apps/Mp4Decrypt/Mp4Decrypt.cpp'],
    ['mp4diff', 'Source/C++/Apps/Mp4Diff/Mp4Diff.cpp'],
    ['mp4dump', 'Source/C++/Apps/Mp4Dump/Mp4Dump.cpp'],
    ['mp4edit', 'Source/C++/Apps/Mp4Edit/Mp4Edit.cpp'],
    ['mp4encrypt', 'Source/C++/Apps/Mp4Encrypt/Mp4Encrypt.cpp'],
    ['mp4extract', 'Source/C++/Apps/Mp4Extract/Mp4Extract.cpp'],
    ['mp4fragment', 'Source/C++/Apps/Mp4Fragment/Mp4Fragment.cpp'],
    ['mp4iframeindex', 'Source/C++/Apps/Mp4IframeIndex/Mp4IframeIndex.cpp'],
    ['mp4info', 'Source/C++/Apps/Mp4Info/Mp4Info.cpp'],
    ['mp4mux', 'Source/C++/Apps/Mp4Mux/Mp4Mux.cpp'],
    ['mp4pssh', 'Source/C++/Apps/Mp4Pssh/Mp4Pssh.cpp'],
    ['mp4rtphintinfo', 'Source/C++/Apps/Mp4RtpHintInfo/Mp4RtpHintInfo.cpp'],
    ['mp4split', 'Source/C++/Apps/Mp4Split/Mp4Split.cpp'],
    ['mp4tag', 'Source/C++/Apps/Mp4Tag/Mp4Tag.cpp']
  ]

  foreach app: apps  
    app_exe = executable(
      app[0],
      app[1],
      dependencies: project_dep,
      install: true
    )

    if not meson.is_subproject()
      if test_apps == 'true' or get_option('buildtype').startswith('debug')
          test('test_' + app[0], app_exe)
      endif
    endif
  endforeach

  if get_option('python-scripts') == 'auto'
    install_data([
        'Source/Python/utils/aes.py',
        'Source/Python/utils/check-indexes.py',
        'Source/Python/utils/mp4-dash-clone.py',
        'Source/Python/utils/mp4-dash-encode.py',
        'Source/Python/utils/mp4-dash.py',
        'Source/Python/utils/mp4-hls.py',
        'Source/Python/utils/mp4utils.py',
        'Source/Python/utils/pr-derive-key.py',
        'Source/Python/utils/skm.py',
        'Source/Python/utils/subtitles.py',
        'Source/Python/utils/wv-request.py'
      ],
      install_dir: 'utils'
    )

    if host_machine.system() == 'windows'
      install_data([
          'Source/Python/wrappers/mp4dash.bat',
          'Source/Python/wrappers/mp4dashclone.bat',
          'Source/Python/wrappers/mp4hls.bat'
        ],
        install_dir: get_option('bindir')
      )
    else
      install_data([
          'Source/Python/wrappers/mp4dash',
          'Source/Python/wrappers/mp4dashclone',
          'Source/Python/wrappers/mp4hls'
        ],
        install_dir: get_option('bindir')
      )
    endif
  endif
endif

if get_option('docs')
  install_data([
      'Documents/Doxygen/Bento4-HTML.zip',
      'Documents/Doxygen/Bento4.chm',
      'Documents/SDK/Bento4_SDK_documentation.doc',
      'Documents/SDK/Bento4_SDK_documentation.pdf',
      'Documents/LICENSE.txt'
    ],
    install_dir: 'docs'
  )
endif
