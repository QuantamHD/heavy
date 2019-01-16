defmodule Heavy.OSX.AudioToolBoxTest do
  use ExUnit.Case

  alias Heavy.OSX.AudioStreamBasicDescription
  alias Heavy.OSX.AudioToolBox

  @asbd %AudioStreamBasicDescription{
    bits_per_channel: 16,
    channels_per_frame: 1,
    bytes_per_packet: 2,
    bytes_per_frame: 2,
    sample_rate: 44100.0,
    frames_per_packet: 1
  }

  test "creating audio file works" do
    file_path = "test/scratch_folder/32fcff32-d110-4e2c-b496-5a10685b889e"
    {:ok, _reference} = AudioToolBox.create_audio_file(file_path, @asbd)
    assert true = File.exists?(file_path)
    File.rm(file_path)
  end

  test "read audio metadata of created file" do
    file_path = "test/scratch_folder/dfd8e2d6-fcd7-4f6d-a19f-e1e35d0ab8b0"

    {:ok, reference} = AudioToolBox.create_audio_file(file_path, @asbd)
    assert true = File.exists?(file_path)

    assert {:ok, %{"approximate duration in seconds" => "0"}} =
             AudioToolBox.read_audio_metadata(reference)

    File.rm(file_path)
  end

  test "write square wave file to aif file" do
    low =
      for _ <- 1..100, into: <<>> do
        <<-32768::big-signed-size(16)>>
      end

    high =
      for _ <- 1..100, into: <<>> do
        <<32767::big-signed-size(16)>>
      end

    signal =
      for i <- 1..10000 do
        if rem(i, 2) == 0 do
          high
        else
          low
        end
      end

    file_path = "test/scratch_folder/dfdd8e2d6-fcd7-4f6d-a19f-e1e35d0ab8b0.aif"

    assert {:ok, reference} = AudioToolBox.create_audio_file(file_path, @asbd)
    assert true = File.exists?(file_path)

    assert :ok = AudioToolBox.write_audio(reference, signal)

    assert {:ok, %{"approximate duration in seconds" => "22.676"}} =
             AudioToolBox.read_audio_metadata(reference)

    assert :ok = AudioToolBox.close_audio_file(reference)

    File.rm(file_path)
  end
end
