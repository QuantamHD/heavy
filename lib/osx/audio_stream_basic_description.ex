defmodule Heavy.OSX.AudioStreamBasicDescription do
  defstruct bits_per_channel: nil,
            bytes_per_frame: nil,
            bytes_per_packet: nil,
            channels_per_frame: nil,
            frames_per_packet: nil,
            sample_rate: nil,
            audio_format: nil,
            audio_format_flags: nil
end
