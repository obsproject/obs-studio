cmake_minimum_required(VERSION 3.28...3.30)

add_executable(obs-vst3-scanner)
add_executable(OBS::vst3-scanner ALIAS obs-vst3-scanner)

find_package(SteinbergVST3 REQUIRED)

target_sources(obs-vst3-scanner PRIVATE vst3-scanner.cpp)

target_link_libraries(obs-vst3-scanner PRIVATE OBS::libobs OBS::vst3-library Steinberg::VST3)

set_target_properties_obs(obs-vst3-scanner PROPERTIES FOLDER plugins/obs-vst3)
