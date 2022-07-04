#
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
#
#  http://aws.amazon.com/apache2.0
#
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.
#


#
# Modified by XMOS Ltd
# https://github.com/xmos/avs-device-sdk
#

if [ -z "$PLATFORM" ]; then
  echo "You should run the setup.sh script."
  exit 1
fi

show_help() {
  echo  'Usage: pi.sh [OPTIONS]'
  echo  ''
  echo  'Optional parameters'
  echo  '  -w <keyword-detector-type> Keyword detector to setup: possible values are S (Sensory), A (Amazon), G (GPIO trigger), H (HID trigger), default is no keyword detector, only tap-to-talk is enabled'
  echo  '  -h                    Display this help and exit'
}
KEY_WORD_DETECTOR_FLAG=""
OPTIONS=w:h
while getopts "$OPTIONS" opt ; do
  case $opt in
    w )
      KEY_WORD_DETECTOR_FLAG="$OPTARG"
      ;;
    h )
      show_help
      exit 1
      ;;
  esac
done

SOUND_CONFIG="$HOME/.asoundrc"
START_SCRIPT="$INSTALL_BASE/startsample.sh"
START_PREVIEW_SCRIPT="$INSTALL_BASE/startpreview.sh"
CMAKE_PLATFORM_SPECIFIC=(-DGSTREAMER_MEDIA_PLAYER=ON \
    -DPORTAUDIO=ON \
    -DPORTAUDIO_LIB_PATH="$THIRD_PARTY_PATH/portaudio/lib/.libs/libportaudio.$LIB_SUFFIX" \
    -DPORTAUDIO_INCLUDE_DIR="$THIRD_PARTY_PATH/portaudio/include" \
    -DCURL_INCLUDE_DIR=${THIRD_PARTY_PATH}/curl-${CURL_VER}/include \
    -DCURL_LIBRARY=${THIRD_PARTY_PATH}/curl-${CURL_VER}/lib/.libs/libcurl.so)

# Add the flags for the different keyword detectors
case $KEY_WORD_DETECTOR_FLAG in
  S )
    # Set CMAKE options for Sensory Keyword detector
    CMAKE_PLATFORM_SPECIFIC+=(-DSENSORY_KEY_WORD_DETECTOR=ON \
         -DSENSORY_OP_POINT_FLAG=ON \
         -DXMOS_AVS_TESTS_FLAG=ON \
         -DSENSORY_KEY_WORD_DETECTOR_LIB_PATH=$THIRD_PARTY_PATH/alexa-rpi/lib/libsnsr.a \
         -DSENSORY_KEY_WORD_DETECTOR_INCLUDE_DIR=$THIRD_PARTY_PATH/alexa-rpi/include)
    ;;
  A )
    # Set CMAKE options for Amazon Keyword detector
    CMAKE_PLATFORM_SPECIFIC+=""
    ;;
  G )
    CMAKE_PLATFORM_SPECIFIC+=(-DGPIO_KEY_WORD_DETECTOR=ON)
    ;;
  H )
    CMAKE_PLATFORM_SPECIFIC+=(-DHID_KEY_WORD_DETECTOR=ON)
    ;;
esac

GSTREAMER_AUDIO_SINK="alsasink"

install_dependencies() {
  sudo apt-get update
  sudo apt-get -y install git gcc cmake build-essential libsqlite3-dev libssl-dev libnghttp2-dev libfaad-dev libsoup2.4-dev libgcrypt20-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-good libasound2-dev sox gedit vim doxygen graphviz
}

run_os_specifics() {
  build_port_audio
  build_curl
  if [ [ -z $KEY_WORD_DETECTOR_FLAG ] ]
  then
    echo
    echo "==============> TAP-TO-TALK IS ENABLED =============="
    echo
  fi
}

configure_sound() {
  echo
  echo "==============> SAVING AUDIO CONFIGURATION FILE =============="
  echo

  cat << EOF > "$SOUND_CONFIG"
  pcm.!default {
    type asym
     playback.pcm {
       type plug
       slave.pcm "hw:0,0"
     }
     capture.pcm {
       type plug
       slave.pcm "hw:1,0"
     }
  }
EOF
}

build_curl() {
  #get curl and build
  echo
  echo "==============> CLONING AND BUILDING CURL =============="
  echo

  cd $THIRD_PARTY_PATH
  wget ${CURL_DOWNLOAD_URL}
  tar xzf curl-${CURL_VER}.tar.gz
  cd curl-${CURL_VER}
  ./configure --with-nghttp2 --prefix=${THIRD_PARTY_PATH}/curl-${CURL_VER} --with-ssl
  make
}

generate_start_script() {
  cat << EOF > "$START_SCRIPT"
  cd "$BUILD_PATH/SampleApp/src"

  PA_ALSA_PLUGHW=1 ./SampleApp "$OUTPUT_CONFIG_FILE" DEBUG9
EOF

  cat << EOF > "$START_PREVIEW_SCRIPT"
  cd "$BUILD_PATH/applications/acsdkPreviewAlexaClient/src"

  PA_ALSA_PLUGHW=1 ./PreviewAlexaClient "$OUTPUT_CONFIG_FILE" DEBUG9
EOF
}

generate_test_script() {
  cat << EOF > "${TEST_SCRIPT}"
  echo
  echo "==============> BUILDING Tests =============="
  echo
  cd $BUILD_PATH
  make all test
EOF
}
