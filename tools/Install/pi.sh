#
# Copyright 2018-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
  echo  '  -G                    Flag to enable keyword detector on GPIO interrupt'
  echo  '  -H                    Flag to enable keyword detector on HID event'
  echo  '  -h                    Display this help and exit'
}
OPTIONS=GHh
while getopts "$OPTIONS" opt ; do
    case $opt in
        G ) 
            GPIO_KEY_WORD_DETECTOR_FLAG="ON"
            ;;
        H ) 
            HID_KEY_WORD_DETECTOR_FLAG="ON"
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
CMAKE_PLATFORM_SPECIFIC=(-DGSTREAMER_MEDIA_PLAYER=ON -DPORTAUDIO=ON \
      -DPORTAUDIO_LIB_PATH="$THIRD_PARTY_PATH/portaudio/lib/.libs/libportaudio.$LIB_SUFFIX" \
      -DPORTAUDIO_INCLUDE_DIR="$THIRD_PARTY_PATH/portaudio/include" \
      -DCURL_INCLUDE_DIR=${THIRD_PARTY_PATH}/curl-${CURL_VER}/include \
      -DCURL_LIBRARY=${THIRD_PARTY_PATH}/curl-${CURL_VER}/lib/.libs/libcurl.so)

# Add the flags for the different keyword detectors
if [ -n "$SENSORY_KEY_WORD_DETECTOR_FLAG" ]
then
  CMAKE_PLATFORM_SPECIFIC+=(-DSENSORY_KEY_WORD_DETECTOR=ON \
        -DSENSORY_KEY_WORD_DETECTOR_LIB_PATH=$THIRD_PARTY_PATH/alexa-rpi/lib/libsnsr.a \
        -DSENSORY_KEY_WORD_DETECTOR_INCLUDE_DIR=$THIRD_PARTY_PATH/alexa-rpi/include)
elif [ -n "$GPIO_KEY_WORD_DETECTOR_FLAG" ]
then
  CMAKE_PLATFORM_SPECIFIC+=(-DGPIO_KEY_WORD_DETECTOR=ON)
elif [ -n "$HID_KEY_WORD_DETECTOR_FLAG" ]
then
  CMAKE_PLATFORM_SPECIFIC+=(-DHID_KEY_WORD_DETECTOR=ON)
fi

GSTREAMER_AUDIO_SINK="alsasink"

install_dependencies() {
  sudo apt-get update
  sudo apt-get -y install git gcc cmake build-essential libsqlite3-dev libssl-dev libnghttp2-dev libfaad-dev libsoup2.4-dev libgcrypt20-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-good libasound2-dev sox gedit vim doxygen graphviz
}

run_os_specifics() {
  build_port_audio
  build_curl
  if [ -n "$SENSORY_KEY_WORD_DETECTOR_FLAG" ]
  then
    build_kwd_engine
  fi
  # the step below is not necessary for any XMOS setup
  # configure_sound
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

build_kwd_engine() {
  #get sensory and build
  echo
  echo "==============> CLONING AND BUILDING SENSORY =============="
  echo

  cd $THIRD_PARTY_PATH
  rm -rf alexa-rpi
  git clone git://github.com/Sensory/alexa-rpi.git
  pushd alexa-rpi > /dev/null
  git checkout $SENSORY_MODEL_HASH -- models/spot-alexa-rpi-31000.snsr
  popd > /dev/null
  bash ./alexa-rpi/bin/license.sh
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

  PA_ALSA_PLUGHW=1 ./SampleApp "$OUTPUT_CONFIG_FILE" "$THIRD_PARTY_PATH/alexa-rpi/models" DEBUG9
EOF

  cat << EOF > "$START_PREVIEW_SCRIPT"
  cd "$BUILD_PATH/applications/acsdkPreviewAlexaClient/src"

  PA_ALSA_PLUGHW=1 ./PreviewAlexaClient "$OUTPUT_CONFIG_FILE" "$THIRD_PARTY_PATH/alexa-rpi/models" DEBUG9
EOF
}

generate_test_script() {
  cat << EOF > "${TEST_SCRIPT}"
  echo
  echo "==============> BUILDING Tests =============="
  echo
  mkdir -p "$UNIT_TEST_MODEL_PATH"
  cp "$UNIT_TEST_MODEL" "$UNIT_TEST_MODEL_PATH"
  cd $BUILD_PATH
  make all test
EOF
}
