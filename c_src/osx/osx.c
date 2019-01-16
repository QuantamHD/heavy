#include <AudioToolbox/AudioFile.h>
#include <AudioToolbox/AudioToolbox.h>
#include "erl_nif.h"
#include "stdio.h"
#include "string.h"
#include "data_conversion.h"

ErlNifResourceType* AUDIO_FILE_ID;

static ERL_NIF_TERM open_audio_file(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  ErlNifBinary address_bin;
  if (!enif_inspect_iolist_as_binary(env, argv[0], &address_bin)) return enif_make_badarg(env);

  char *address = strndup((char*) address_bin.data, address_bin.size);

  CFStringRef ref = CFStringCreateWithCString(NULL, address, kCFStringEncodingUTF8); // MUST_FREE
  CFURLRef file_url = CFURLCreateWithFileSystemPath(NULL, ref, kCFURLPOSIXPathStyle, false); // MUST_FREE
  AudioFileID* file_id = enif_alloc_resource(AUDIO_FILE_ID, sizeof(AudioFileID));
  OSStatus err = noErr;

  err = AudioFileOpenURL(file_url, kAudioFileReadPermission, 0, file_id);
  if (err != noErr) return create_osstatus_error(env, err);

  ERL_NIF_TERM resource = enif_make_resource(env, file_id);

  enif_release_resource(file_id);

  CFRelease(ref);
  CFRelease(file_url);
  free(address);

  return resource;
}

static ERL_NIF_TERM create_audio_file(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  ErlNifBinary address_bin;
  if (!enif_inspect_iolist_as_binary(env, argv[0], &address_bin)) return enif_make_badarg(env);
  if (!enif_is_map(env, argv[1])) return enif_make_badarg(env);
  if (!enif_is_atom(env, argv[2])) return enif_make_badarg(env);

  char *address = strndup((char*) address_bin.data, address_bin.size);//MUST_FREE

  CFStringRef ref = CFStringCreateWithCString(NULL, address, kCFStringEncodingUTF8); // MUST_FREE
  CFURLRef file_url = CFURLCreateWithFileSystemPath(NULL, ref, kCFURLPOSIXPathStyle, false); // MUST_FREE
  AudioFileID* file_id = enif_alloc_resource(AUDIO_FILE_ID, sizeof(AudioFileID));
  OSStatus err = noErr;

  AudioStreamBasicDescription absd;
  memset(&absd, 0, sizeof(absd));

  if(!from_struct_to_audio_stream_basic_description(env, argv[1], &absd)) {
    CFRelease(ref);
    CFRelease(file_url);
    free(address);
    return enif_make_error_tuple(env, enif_make_atom(env, "invalid_audio_stream_description"));
  }

  AudioFileTypeID audio_file_type_id;
  if(!from_atom_to_AudioFileTypeID(env, argv[2], &audio_file_type_id)){
    CFRelease(ref);
    CFRelease(file_url);
    free(address);
    return enif_make_error_tuple(env, enif_make_atom(env, "invalid_audio_file_type"));
  }
  
  err = AudioFileCreateWithURL(file_url, audio_file_type_id, &absd, kAudioFileFlags_EraseFile, file_id);
  if (err != noErr){
    CFRelease(ref);
    CFRelease(file_url);
    free(address);
    return create_osstatus_error(env, err);
  }

  ERL_NIF_TERM resource = enif_make_resource(env, file_id);

  enif_release_resource(file_id);

  CFRelease(ref);
  CFRelease(file_url);
  free(address);
  return enif_make_tuple2(env, enif_make_atom(env, "ok"), resource);
}

static ERL_NIF_TERM read_audio_metadata(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
  OSStatus err = noErr;
  AudioFileID* file_id_ptr;
  enif_get_resource(env, argv[0], AUDIO_FILE_ID, (void **)&file_id_ptr);

  AudioFileID file_id = *file_id_ptr;

  uint32_t dict_size = 0;
  CFDictionaryRef dictionary_ref; // MUST_FREE
  err = AudioFileGetPropertyInfo(file_id, kAudioFilePropertyInfoDictionary, &dict_size, 0);
  if (err != noErr) return create_osstatus_error(env, err);


  err = AudioFileGetProperty(file_id, kAudioFilePropertyInfoDictionary, &dict_size, &dictionary_ref);
  if (err != noErr) return create_osstatus_error(env, err);

  ERL_NIF_TERM map_term = cftype_to_term(env, dictionary_ref);

  CFRelease(dictionary_ref);

  return enif_make_tuple2(env, enif_make_atom(env, "ok"), map_term);
}

static ERL_NIF_TERM write_audio(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
  OSStatus err = noErr;
  AudioFileID* file_id_ptr;

  ErlNifBinary address_bin;
  if (!enif_inspect_iolist_as_binary(env, argv[1], &address_bin)) return enif_make_badarg(env);

  enif_get_resource(env, argv[0], AUDIO_FILE_ID, (void **)&file_id_ptr);
  AudioFileID file_id = *file_id_ptr;

  uint64_t byte_offset;
  enif_get_uint64(env, argv[2], byte_offset);

  uint32_t size = (uint32_t)address_bin.size;
  err = AudioFileWriteBytes(file_id, false, byte_offset, &size, address_bin.data);
  if (err != noErr) return create_osstatus_error(env, err);

  return enif_make_atom(env, "ok");
}

static ERL_NIF_TERM close_audio_file(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
  OSStatus err = noErr;
  AudioFileID* file_id_ptr;

  enif_get_resource(env, argv[0], AUDIO_FILE_ID, (void **)&file_id_ptr);
  AudioFileID file_id = *file_id_ptr;

  err = AudioFileClose(file_id);
  if (err != noErr) return create_osstatus_error(env, err);

  return enif_make_atom(env, "ok");
}

static ERL_NIF_TERM avaliable_stream_descriptions(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
  if (!enif_is_atom(env, argv[0])) return enif_make_badarg(env);
  if (!enif_is_atom(env, argv[1])) return enif_make_badarg(env);

  OSStatus err = noErr;

  AudioFileTypeAndFormatID type_and_format_id;
  
  if(!from_atom_to_AudioFileTypeID(env, argv[0], &type_and_format_id.mFileType)){
    return enif_make_error_tuple(env, enif_make_atom(env, "invalid_file_type"));
  }

  if(!from_atom_to_AudioFormatID(env, argv[1], &type_and_format_id.mFormatID)){
    return enif_make_error_tuple(env, enif_make_atom(env, "invalid_audio_format"));
  }

  uint32_t format_size = 0; 
  err = AudioFileGetGlobalInfoSize(
    kAudioFileGlobalInfo_AvailableStreamDescriptionsForFormat, 
    sizeof(type_and_format_id), 
    &type_and_format_id, 
    &format_size);
  if(err != noErr) return create_osstatus_error(env, err);

  AudioStreamBasicDescription *asbds = malloc(format_size);

  err = AudioFileGetGlobalInfo(
    kAudioFileGlobalInfo_AvailableStreamDescriptionsForFormat, 
    sizeof(type_and_format_id), 
    &type_and_format_id, 
    &format_size, 
    asbds);
  if(err != noErr) return create_osstatus_error(env, err);

  int absd_count = format_size / sizeof(AudioStreamBasicDescription);
  ERL_NIF_TERM* list_items = malloc(absd_count * sizeof(ERL_NIF_TERM));
  
  for(int i = 0; i < absd_count; i++) {
    list_items[i] = from_audio_stream_basic_description_to_struct(env, asbds[i]);
  }

  free(asbds);
  return enif_make_list_from_array(env, list_items, absd_count);
}

static void MyAQInputCallback(void *inUserData, AudioQueueRef inQueue,
							  AudioQueueBufferRef inBuffer,
							  const AudioTimeStamp *inStartTime,
							  UInt32 inNumPackets,
							  const AudioStreamPacketDescription *inPacketDesc)
{
  ErlNifEnv* env =  enif_alloc_env();
  ErlNifPid* pid = (ErlNifPid*) inUserData;
  ErlNifBinary binary;
  enif_alloc_binary(inBuffer->mAudioDataByteSize, &binary);
  memcpy(binary.data, inBuffer->mAudioData, inBuffer->mAudioDataByteSize);
  enif_send(NULL, pid, env, enif_make_binary(env, &binary));
  AudioQueueEnqueueBuffer(inQueue, inBuffer, 0, NULL);
}

static ERL_NIF_TERM record_audio(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
  OSStatus err = noErr;

  AudioQueueRef queue = {0};
  AudioStreamBasicDescription asbd;
  from_struct_to_audio_stream_basic_description(env, argv[0], &asbd);

  ErlNifPid* pid = malloc(sizeof(pid));
  enif_self(env, pid);

  err = AudioQueueNewInput(&asbd, MyAQInputCallback, pid, NULL, NULL, 0, &queue);
  if(err != noErr) return create_osstatus_error(env, err);

  for (int i  = 0; i < 3; i++)
	{
		AudioQueueBufferRef buffer;
		err = AudioQueueAllocateBuffer(queue, 2000, &buffer);
    if(err != noErr) return create_osstatus_error(env, err);
		err = AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
    if(err != noErr) return create_osstatus_error(env, err);
	}

  err = AudioQueueStart(queue, NULL);
  if(err != noErr) return create_osstatus_error(env, err);

  uint32_t size = sizeof(asbd);
  AudioQueueGetProperty(queue, kAudioConverterCurrentOutputStreamDescription, &asbd, &size);

  return from_audio_stream_basic_description_to_struct(env, asbd);
}

int load(ErlNifEnv* env, void **priv_data, ERL_NIF_TERM load_info) {
  ErlNifResourceFlags flags;
  AUDIO_FILE_ID = enif_open_resource_type(env, NULL, "AudioFileId", NULL, ERL_NIF_RT_CREATE, &flags);
  
  return 0;
}

static ErlNifFunc heavy_nif_funcs[] =
{
	{"read_audio_metadata", 1, read_audio_metadata, ERL_NIF_DIRTY_JOB_IO_BOUND},
  {"open_audio_file", 1, open_audio_file, ERL_NIF_DIRTY_JOB_IO_BOUND},
  {"create_audio_file", 3, create_audio_file, ERL_NIF_DIRTY_JOB_IO_BOUND},
  {"write_audio", 3, write_audio, ERL_NIF_DIRTY_JOB_IO_BOUND},
  {"close_audio_file", 1, close_audio_file, ERL_NIF_DIRTY_JOB_IO_BOUND},
  {"avaliable_stream_descriptions", 2, avaliable_stream_descriptions, ERL_NIF_DIRTY_JOB_IO_BOUND},
  {"record_audio", 1, record_audio, ERL_NIF_DIRTY_JOB_IO_BOUND},
};

ERL_NIF_INIT(Elixir.Heavy.OSX.AudioToolBox, heavy_nif_funcs, load, NULL, NULL, NULL)
