#include "erl_nif.h"
#include "string.h"
#include <CoreFoundation/CoreFoundation.h>
#include <AudioToolbox/AudioFile.h>
#include <AudioToolbox/AudioToolbox.h>


/*
Forward Declarations for the CFType_to_NIF function.

This forward deceleration is required because there is a codependant relationship
between this function and some of the other cf_conversion functions. Specifically the
array and dictionary types as examples since a CFDictionary can hold other types of CFTypes.
To convert these types the convert_cf_map function calls out to CFType_to_NIF to convert them
to Erlang terms.
*/
ERL_NIF_TERM cftype_to_term(ErlNifEnv* env, CFTypeRef ref);


#define get_required_map_uint(env, map, key, integer_var, error_atom) do {\
  if(1) {\
    ERL_NIF_TERM dea9ff60d95d__map_value_term;\
    if(enif_get_map_value(env, map, key, &dea9ff60d95d__map_value_term) && enif_get_uint(env, dea9ff60d95d__map_value_term, integer_var)){ \
      ; \
    } else\
      return enif_make_error_tuple(env, enif_make_atom(env, error_atom));\
  } \
} while(0) 

#define get_required_map_double(env, map, key, double_var, error_atom) do {\
  if(1) {\
    ERL_NIF_TERM dea9ff60d95d__map_value_term;\
    if(enif_get_map_value(env, map, key, &dea9ff60d95d__map_value_term) && enif_get_double(env, dea9ff60d95d__map_value_term, double_var)){ \
      ; \
    } else\
      return enif_make_error_tuple(env, enif_make_atom(env, error_atom));\
  } \
} while(0) 



ERL_NIF_TERM enif_make_error_tuple(ErlNifEnv* env, ERL_NIF_TERM term) {
  return enif_make_tuple2(env, enif_make_atom(env, "error"), term);
}

ERL_NIF_TERM create_osstatus_error(ErlNifEnv* env, OSStatus osstatus) {
  return enif_make_error_tuple(
    env, 
    enif_make_tuple2(
      env, 
      enif_make_atom(env, "osstaus"),
      enif_make_int(env, osstatus)
    )
  );
}

ERL_NIF_TERM convert_cf_string(ErlNifEnv* env, CFStringRef ref) {
  uint8_t* string_data = (uint8_t*) CFStringGetCStringPtr(ref, kCFStringEncodingUTF8);

  if(string_data != NULL){
    ERL_NIF_TERM string_term;

    size_t string_size = strlen((const char*)string_data);
    uint8_t* string_binary = enif_make_new_binary(env, string_size, &string_term);
    memcpy(string_binary, string_data, string_size);

    return string_term;
  }

  CFIndex number_of_unicode_points = CFStringGetLength(ref);
  CFIndex maximum_length_in_utf8   = CFStringGetMaximumSizeForEncoding(number_of_unicode_points, kCFStringEncodingUTF8);
  char string_bytes_utf8[maximum_length_in_utf8 + 1]; // +1 For the NUL Byte
  if (CFStringGetCString(ref, string_bytes_utf8, maximum_length_in_utf8, kCFStringEncodingUTF8)) {
    ERL_NIF_TERM erl_binary;

    size_t string_size = strlen(string_bytes_utf8);
    unsigned char* erl_binary_buffer = enif_make_new_binary(env, string_size, &erl_binary);
    memcpy(erl_binary_buffer, string_bytes_utf8, string_size);

    return erl_binary;
  }

  return enif_make_error_tuple(env, enif_make_atom(env, "failed_to_translate_cfstring"));
}

ERL_NIF_TERM convert_cf_map(ErlNifEnv* env, CFDictionaryRef ref) {

  CFIndex number_of_keys_and_values = CFDictionaryGetCount(ref);
  
  const void* keys[number_of_keys_and_values];
  const void* values[number_of_keys_and_values];

  ERL_NIF_TERM erl_keys[number_of_keys_and_values];
  ERL_NIF_TERM erl_values[number_of_keys_and_values];

  CFDictionaryGetKeysAndValues(ref, keys, values);
  for(int i = 0; i < number_of_keys_and_values; i++) {
    erl_keys[i] = cftype_to_term(env, keys[i]);
    erl_values[i] = cftype_to_term(env, values[i]);
  }

  ERL_NIF_TERM map;
  enif_make_map_from_arrays(env, erl_keys, erl_values, number_of_keys_and_values, &map);

  return map;
}

ERL_NIF_TERM cftype_to_term(ErlNifEnv* env, CFTypeRef ref) {
  CFTypeID type_id = CFGetTypeID(ref);
  if (CFDictionaryGetTypeID() == type_id){
    return convert_cf_map(env, (CFDictionaryRef) ref);
  } else if(CFStringGetTypeID() == type_id) {
    return convert_cf_string(env, (CFStringRef) ref);
  }
  
  return enif_make_tuple2(env, enif_make_atom(env, "cf_type_unimplended_this_is_a_bug"), enif_make_uint64(env, type_id));
}

uint32_t get_int32_map(ErlNifEnv* env, ERL_NIF_TERM map, ERL_NIF_TERM key, uint32_t default_value) {
  ERL_NIF_TERM map_value;
  uint32_t map_integer;
  if(!enif_get_map_value(env, map, key, &map_value) && enif_get_uint(env, map_value, &map_integer)) {
    return map_integer;
  }
  return default_value;
}

boolean_t get_require_map_uint_b(ErlNifEnv* env, ERL_NIF_TERM erl_struct, const char* key, uint32_t* value) {
  ERL_NIF_TERM map_value_term;
  if(enif_get_map_value(env, erl_struct, enif_make_atom(env, key), &map_value_term) && enif_get_uint(env, map_value_term, value)){
    return true;
  } 
  return false;
}

boolean_t get_require_map_double_b(ErlNifEnv* env, ERL_NIF_TERM erl_struct, const char* key, double* value) {
  ERL_NIF_TERM map_value_term;
  if(enif_get_map_value(env, erl_struct, enif_make_atom(env, key), &map_value_term) && enif_get_double(env, map_value_term, value)){
    return true;
  } 
  return false;
}

boolean_t get_map_boolean(ErlNifEnv* env, ERL_NIF_TERM erl_struct, const char* key) {
  ERL_NIF_TERM map_value_term;
  if(enif_get_map_value(env, erl_struct, enif_make_atom(env, key), &map_value_term) && enif_compare(map_value_term, enif_make_atom(env, "true")) == 0){
    return true;
  } 
  return false;
}

boolean_t from_atom_to_AudioFileTypeID(ErlNifEnv* env, ERL_NIF_TERM atom, AudioFileTypeID* audio_file_id) {
  if(enif_compare(atom, enif_make_atom(env, "aiff")) == 0){
    *audio_file_id = kAudioFileAIFFType;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "aifc")) == 0){
    *audio_file_id = kAudioFileAIFCType;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "wav")) == 0){
    *audio_file_id = kAudioFileWAVEType;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "rf64")) == 0){
    *audio_file_id = kAudioFileRF64Type;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "sd2")) == 0){
    *audio_file_id = kAudioFileSoundDesigner2Type;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mp3")) == 0){
    *audio_file_id = kAudioFileMP3Type;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "next")) == 0){
    *audio_file_id = kAudioFileNextType;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mp2")) == 0){
    *audio_file_id = kAudioFileMP2Type;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mp1")) == 0){
    *audio_file_id = kAudioFileMP1Type;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "ac3")) == 0){
    *audio_file_id = kAudioFileAC3Type;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "adts")) == 0){
    *audio_file_id = kAudioFileAAC_ADTSType;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mp4")) == 0){
    *audio_file_id = kAudioFileMPEG4Type;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "m4af")) == 0){
    *audio_file_id = kAudioFileM4AType;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "m4bf")) == 0){
    *audio_file_id = kAudioFileM4BType;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "caf")) == 0){
    *audio_file_id = kAudioFileCAFType;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "3gp")) == 0){
    *audio_file_id = kAudioFile3GPType;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "3gp2")) == 0){
    *audio_file_id = kAudioFile3GP2Type;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "amr")) == 0){
    *audio_file_id = kAudioFileAMRType;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "flac")) == 0){
    *audio_file_id = kAudioFileFLACType;
    return true;
  } 
  return false;
}

boolean_t from_AudioFileTypeID_to_atom(ErlNifEnv* env, AudioFileTypeID audio_file_id, ERL_NIF_TERM* atom) {
  switch(audio_file_id) {
    case kAudioFileAIFFType :
      *atom = enif_make_atom(env, "aiff");
      return true;
    case kAudioFileAIFCType :
      *atom = enif_make_atom(env, "aifc");
      return true;
    case kAudioFileWAVEType :
      *atom = enif_make_atom(env, "wav");
      return true;
    case kAudioFileRF64Type :
      *atom = enif_make_atom(env, "rf64");
      return true;
    case kAudioFileSoundDesigner2Type :
      *atom = enif_make_atom(env, "sd2");
      return true;
    case kAudioFileNextType :
      *atom = enif_make_atom(env, "next");
      return true;
    case kAudioFileMP3Type :
      *atom = enif_make_atom(env, "mp3");
      return true; 
    case kAudioFileMP2Type :
      *atom = enif_make_atom(env, "mp2");
      return true; 
    case kAudioFileMP1Type :
      *atom = enif_make_atom(env, "mp1");
      return true; 
    case kAudioFileAC3Type :
      *atom = enif_make_atom(env, "ac3");
      return true; 
    case kAudioFileAAC_ADTSType :
      *atom = enif_make_atom(env, "adts");
      return true; 
    case kAudioFileMPEG4Type :
      *atom = enif_make_atom(env, "mp4");
      return true; 
    case kAudioFileM4AType :
      *atom = enif_make_atom(env, "m4a");
      return true; 
    case kAudioFileM4BType :
      *atom = enif_make_atom(env, "m4b");
      return true; 
    case kAudioFileCAFType :
      *atom = enif_make_atom(env, "caf");
      return true; 
    case kAudioFile3GPType :
      *atom = enif_make_atom(env, "3gp");
      return true; 
    case kAudioFile3GP2Type :
      *atom = enif_make_atom(env, "3gp2");
      return true; 
    case kAudioFileAMRType :
      *atom = enif_make_atom(env, "amr");
      return true; 
    case kAudioFileFLACType :
      *atom = enif_make_atom(env, "flac");
      return true;
  }
  return false;
}

boolean_t from_AudioFormatID_to_atom(ErlNifEnv* env, AudioFormatID audio_format_id, ERL_NIF_TERM* atom) {
  switch(audio_format_id) {
    case kAudioFormatLinearPCM :
      *atom = enif_make_atom(env, "lpcm");
      return true;
    case kAudioFormatAC3 :
      *atom = enif_make_atom(env, "ac3");
      return true;
    case kAudioFormat60958AC3 :
      *atom = enif_make_atom(env, "60958ac3");
      return true;
    case kAudioFormatAppleIMA4 :
      *atom = enif_make_atom(env, "apple_IMA4");
      return true;
    case kAudioFormatMPEG4AAC :
      *atom = enif_make_atom(env, "mpeg4_acc");
      return true;
    case kAudioFormatMPEG4CELP :
      *atom = enif_make_atom(env, "mpeg4_celp");
      return true;
    case kAudioFormatMPEG4HVXC :
      *atom = enif_make_atom(env, "mpeg4_hvxc");
      return true; 
    case kAudioFormatMPEG4TwinVQ :
      *atom = enif_make_atom(env, "mpeg4_twin_vq");
      return true; 
    case kAudioFormatMACE3 :
      *atom = enif_make_atom(env, "mace3");
      return true; 
    case kAudioFormatMACE6 :
      *atom = enif_make_atom(env, "mace6");
      return true; 
    case kAudioFormatULaw :
      *atom = enif_make_atom(env, "ulaw");
      return true; 
    case kAudioFormatALaw :
      *atom = enif_make_atom(env, "alaw");
      return true; 
    case kAudioFormatQDesign :
      *atom = enif_make_atom(env, "q_design");
      return true; 
    case kAudioFormatQDesign2 :
      *atom = enif_make_atom(env, "q_design2");
      return true; 
    case kAudioFormatQUALCOMM :
      *atom = enif_make_atom(env, "qualcomm");
      return true; 
    case kAudioFormatMPEGLayer1 :
      *atom = enif_make_atom(env, "mp1");
      return true; 
    case kAudioFormatMPEGLayer2 :
      *atom = enif_make_atom(env, "mp2");
      return true; 
    case kAudioFormatMPEGLayer3 :
      *atom = enif_make_atom(env, "mp3");
      return true; 
    case kAudioFormatTimeCode :
      *atom = enif_make_atom(env, "time_code");
      return true;
    case kAudioFormatMIDIStream :
      *atom = enif_make_atom(env, "midi");
      return true;
    case kAudioFormatParameterValueStream :
      *atom = enif_make_atom(env, "parameter_value_stream");
      return true;
    case kAudioFormatAppleLossless :
      *atom = enif_make_atom(env, "apple_lossless");
      return true;
    case kAudioFormatMPEG4AAC_HE :
      *atom = enif_make_atom(env, "mpeg4_acc_he");
      return true;
    case kAudioFormatMPEG4AAC_LD :
      *atom = enif_make_atom(env, "mpeg4_acc_ld");
      return true;
    case kAudioFormatMPEG4AAC_ELD :
      *atom = enif_make_atom(env, "mpeg4_acc_eld");
      return true;
    case kAudioFormatMPEG4AAC_ELD_SBR :
      *atom = enif_make_atom(env, "mpeg4_acc_eld_sbr");
      return true;
    case kAudioFormatMPEG4AAC_ELD_V2 :
      *atom = enif_make_atom(env, "mpeg4_acc_eld_v2");
      return true;
    case kAudioFormatMPEG4AAC_HE_V2 :
      *atom = enif_make_atom(env, "mpeg4_acc_he_v2");
      return true;
    case kAudioFormatMPEG4AAC_Spatial :
      *atom = enif_make_atom(env, "mpeg4_acc_spatial");
      return true;
    case kAudioFormatAMR :
      *atom = enif_make_atom(env, "amr");
      return true;
    case kAudioFormatAMR_WB :
      *atom = enif_make_atom(env, "amr_wb");
      return true;
    case kAudioFormatAudible :
      *atom = enif_make_atom(env, "audible");
      return true;
    case kAudioFormatiLBC :
      *atom = enif_make_atom(env, "iLBC");
      return true;
    case kAudioFormatDVIIntelIMA :
      *atom = enif_make_atom(env, "DVI_Intel_IMA");
      return true;
    case kAudioFormatMicrosoftGSM :
      *atom = enif_make_atom(env, "Microsoft_GSM");
      return true;
    case kAudioFormatAES3 :
      *atom = enif_make_atom(env, "aes3");
      return true;
    case kAudioFormatEnhancedAC3 :
      *atom = enif_make_atom(env, "enhanced_aes3");
      return true;
    case kAudioFormatFLAC :
      *atom = enif_make_atom(env, "flac");
      return true;
    case kAudioFormatOpus :
      *atom = enif_make_atom(env, "opus");
      return true;
  }
  return false;
}

boolean_t from_atom_to_AudioFormatID(ErlNifEnv* env, ERL_NIF_TERM atom, AudioFormatID* audio_file_id) {
  if(enif_compare(atom, enif_make_atom(env, "lpcm")) == 0){
    *audio_file_id = kAudioFormatLinearPCM;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "ac3")) == 0){
    *audio_file_id = kAudioFormatAC3;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "60958ac3")) == 0){
    *audio_file_id = kAudioFormat60958AC3;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "apple_IMA4")) == 0){
    *audio_file_id = kAudioFormatAppleIMA4;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mpeg4_acc")) == 0){
    *audio_file_id = kAudioFormatMPEG4AAC;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mpeg4_celp")) == 0){
    *audio_file_id = kAudioFormatMPEG4CELP;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mpeg4_hvxc")) == 0){
    *audio_file_id = kAudioFormatMPEG4HVXC;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mpeg4_twin_vq")) == 0){
    *audio_file_id = kAudioFormatMPEG4TwinVQ;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mace3")) == 0){
    *audio_file_id = kAudioFormatMACE3;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mace6")) == 0){
    *audio_file_id = kAudioFormatMACE6;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "ulaw")) == 0){
    *audio_file_id = kAudioFormatULaw;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "alaw")) == 0){
    *audio_file_id = kAudioFormatALaw;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "q_design")) == 0){
    *audio_file_id = kAudioFormatQDesign;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "q_design2")) == 0){
    *audio_file_id = kAudioFormatQDesign2;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "qualcomm")) == 0){
    *audio_file_id = kAudioFormatQUALCOMM;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mp1")) == 0){
    *audio_file_id = kAudioFormatMPEGLayer1;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mp2")) == 0){
    *audio_file_id = kAudioFormatMPEGLayer2;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mp3")) == 0){
    *audio_file_id = kAudioFormatMPEGLayer3;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "time_code")) == 0){
    *audio_file_id = kAudioFormatTimeCode;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "midi")) == 0){
    *audio_file_id = kAudioFormatMIDIStream;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "parameter_value_stream")) == 0){
    *audio_file_id = kAudioFormatParameterValueStream;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "apple_lossless")) == 0){
    *audio_file_id = kAudioFormatAppleLossless;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mpeg4_acc_he")) == 0){
    *audio_file_id = kAudioFormatMPEG4AAC_HE;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mpeg4_acc_ld")) == 0){
    *audio_file_id = kAudioFormatMPEG4AAC_LD;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mpeg4_acc_eld")) == 0){
    *audio_file_id = kAudioFormatMPEG4AAC_ELD;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mpeg4_acc_eld_sbr")) == 0){
    *audio_file_id = kAudioFormatMPEG4AAC_ELD_SBR;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mpeg4_acc_eld_v2")) == 0){
    *audio_file_id = kAudioFormatMPEG4AAC_ELD_V2;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mpeg4_acc_he_v2")) == 0){
    *audio_file_id = kAudioFormatMPEG4AAC_HE_V2;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "mpeg4_acc_spatial")) == 0){
    *audio_file_id = kAudioFormatMPEG4AAC_Spatial;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "amr")) == 0){
    *audio_file_id = kAudioFormatAMR;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "amr_wb")) == 0){
    *audio_file_id = kAudioFormatAMR_WB;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "audible")) == 0){
    *audio_file_id = kAudioFormatAudible;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "iLBC")) == 0){
    *audio_file_id = kAudioFormatiLBC;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "DVI_Intel_IMA")) == 0){
    *audio_file_id = kAudioFormatDVIIntelIMA;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "Microsoft_GSM")) == 0){
    *audio_file_id = kAudioFormatMicrosoftGSM;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "aes3")) == 0){
    *audio_file_id = kAudioFormatAES3;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "enhanced_aes3")) == 0){
    *audio_file_id = kAudioFormatEnhancedAC3;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "flac")) == 0){
    *audio_file_id = kAudioFormatFLAC;
    return true;
  } else if(enif_compare(atom, enif_make_atom(env, "opus")) == 0){
    *audio_file_id = kAudioFormatOpus;
    return true;
  }

  return false;
}

boolean_t from_struct_to_audio_stream_basic_description(ErlNifEnv* env, ERL_NIF_TERM erl_struct, AudioStreamBasicDescription* absd_result) {
  AudioStreamBasicDescription absd;
  memset(&absd, 0, sizeof(absd));

  if(!get_require_map_uint_b(env, erl_struct, "bits_per_channel", &absd.mBitsPerChannel)){return false;}
  if(!get_require_map_uint_b(env, erl_struct, "bytes_per_frame", &absd.mBytesPerFrame)){return false;}
  if(!get_require_map_uint_b(env, erl_struct, "bytes_per_packet", &absd.mBytesPerPacket)){return false;}
  if(!get_require_map_uint_b(env, erl_struct, "channels_per_frame", &absd.mChannelsPerFrame)){return false;}
  if(!get_require_map_uint_b(env, erl_struct, "frames_per_packet", &absd.mFramesPerPacket)){return false;}
  if(!get_require_map_double_b(env, erl_struct, "sample_rate", &absd.mSampleRate)){return false;}
  if(!get_require_map_uint_b(env, erl_struct, "audio_format_flags", &absd.mFormatFlags)){return false;}

  
  ERL_NIF_TERM audio_format;
  if(enif_get_map_value(env, erl_struct, enif_make_atom(env, "audio_format"), &audio_format)){
    AudioFormatID audio_format_id;
    if(from_atom_to_AudioFormatID(env, audio_format, &audio_format_id)){
      absd.mFormatID = audio_format_id;
    }else{
      return false;
    }
  }else{
    return false;
  }

  *absd_result = absd;

  return true;
}

ERL_NIF_TERM from_audio_stream_basic_description_to_struct(ErlNifEnv* env, AudioStreamBasicDescription absd) {
  ERL_NIF_TERM map = enif_make_new_map(env);

  enif_make_map_put(env, map, enif_make_atom(env, "bits_per_channel"), enif_make_uint(env, absd.mBitsPerChannel), &map);
  enif_make_map_put(env, map, enif_make_atom(env, "bytes_per_frame"), enif_make_uint(env, absd.mBytesPerFrame), &map);
  enif_make_map_put(env, map, enif_make_atom(env, "bytes_per_packet"), enif_make_uint(env, absd.mBytesPerPacket), &map);
  enif_make_map_put(env, map, enif_make_atom(env, "channels_per_frame"), enif_make_uint(env, absd.mChannelsPerFrame), &map);
  enif_make_map_put(env, map, enif_make_atom(env, "frames_per_packet"), enif_make_uint(env, absd.mFramesPerPacket), &map);
  enif_make_map_put(env, map, enif_make_atom(env, "sample_rate"), enif_make_double(env, absd.mSampleRate), &map);
  enif_make_map_put(env, map, enif_make_atom(env, "__struct__"), enif_make_atom(env, "Elixir.Heavy.OSX.AudioStreamBasicDescription"), &map);
  
  ERL_NIF_TERM format;
  if(from_AudioFormatID_to_atom(env, absd.mFormatID, &format)){
    enif_make_map_put(env, map, enif_make_atom(env, "audio_format"), format, &map);
  }else{
    return enif_make_error_tuple(env, enif_make_atom(env, "unrecognized_audio_format"));
  }

  enif_make_map_put(env, map, enif_make_atom(env, "audio_format_flags"), enif_make_uint(env, absd.mFormatFlags), &map);


  return map;
}