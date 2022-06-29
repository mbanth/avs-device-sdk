#!/bin/bash

# This is used to create a header file that contains binary data from a file that can be used in this library.  The
# files are downloaded from https://developer.amazon.com/docs/alexa/alexa-voice-service/ux-design-overview.html#sounds.

if [ "$#" -ne 2 ] || ! [ -f "${1}" ] || [ -f "${2}" ]; then
  echo "Usage: $0 <file> <output_header_file>" >&2
  exit 1
fi

FILE=${1}
FULL_OUTPUT=${2}
FILE_NAME=`echo $(basename ${FILE}) | sed -e 's/[^a-zA-Z0-9\]/_/g'`
OUTPUT_FILE=$(basename ${FULL_OUTPUT})
GUARD=`echo "ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_RESOURCES_AUDIO_INCLUDE_AUDIO_DATA_${OUTPUT_FILE}" | tr '[:lower:]' '[:upper:]' | sed -e 's/[^a-zA-Z0-9\]/_/g' | sed -e 's/$/_/g'`
CURR_YEAR=`date +"%Y"`

cat <<EOF > "${FULL_OUTPUT}"
/*
 * ******************
 * ALEXA AUDIO ASSETS
 * ******************
 *
 * Copyright ${CURR_YEAR} Amazon.com, Inc. or its affiliates ("Amazon").
 * All Rights Reserved.
 *
 * These materials are licensed to you as “AVS Materials" under the Amazon
 * Developer Services Agreement, which is currently available at
 * https://developer.amazon.com/support/legal/da
 */
EOF

echo "" >> ${FULL_OUTPUT}
echo "#ifndef ${GUARD}" >> ${FULL_OUTPUT}
echo "#define ${GUARD}" >> ${FULL_OUTPUT}
echo "" >> ${FULL_OUTPUT}
echo "namespace alexaClientSDK {" >> ${FULL_OUTPUT}
echo "namespace applicationUtilities {" >> ${FULL_OUTPUT}
echo "namespace resources {" >> ${FULL_OUTPUT}
echo "namespace audio {" >> ${FULL_OUTPUT}
echo "namespace data {" >> ${FULL_OUTPUT}
echo "" >> ${FULL_OUTPUT}
echo "// clang-format off" >> ${FULL_OUTPUT}
xxd -i ${FILE} >> ${FULL_OUTPUT}
sed -i '' -e 's/unsigned int/constexpr unsigned int/' ${FULL_OUTPUT}
echo "constexpr const char* ${FILE_NAME}_mimetype = \"$(file --mime-type -b ${FILE})\";" >> ${FULL_OUTPUT}
echo "// clang-format on" >> ${FULL_OUTPUT}
echo "" >> ${FULL_OUTPUT}
echo "}  // namespace data" >> ${FULL_OUTPUT}
echo "}  // namespace audio" >> ${FULL_OUTPUT}
echo "}  // namespace resources" >> ${FULL_OUTPUT}
echo "}  // namespace applicationUtilities" >> ${FULL_OUTPUT}
echo "}  // namespace alexaClientSDK" >> ${FULL_OUTPUT}
echo "" >> ${FULL_OUTPUT}
echo "#endif  // ${GUARD}" >> ${FULL_OUTPUT}
