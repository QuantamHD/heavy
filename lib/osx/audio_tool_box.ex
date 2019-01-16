defmodule Heavy.OSX.AudioToolBox do
  @on_load {:init, 0}

  def init do
    :erlang.load_nif("priv/heavy_nif", 0)
  end

  def read_audio_metadata(_) do
    raise "Nif could not be loaded."
  end

  def open_audio_file(_) do
    raise "Nif could not be loaded."
  end

  def close_audio_file(_) do
    raise "Nif could not be loaded."
  end

  def create_audio_file(_file_name, _audio_stream_basic_description, _file_type) do
    raise "Nif could not be loaded."
  end

  def write_audio(_file, _data, _offset) do
    raise "Nif could not be loaded."
  end

  def avaliable_stream_descriptions(_file_type, _format_type) do
    raise "Nif could not be loaded."
  end

  def record_audio(_asbd) do
    raise "Poo"
  end
end
