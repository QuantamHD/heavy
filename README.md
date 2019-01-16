# Heavy


Example Usage

This is a recording audio use case. It will send messages to the process that 
called the function.
```elixir
%Heavy.OSX.AudioStreamBasicDescription{
    audio_format: :lpcm,
    audio_format_flags: 14,
    bits_per_channel: 32,
    bytes_per_frame: 4,
    bytes_per_packet: 4,
    channels_per_frame: 1,
    frames_per_packet: 1,
    sample_rate: 44100.0
}|> Heavy.OSX.AudioToolBox.record_audio()
```

## Installation

If [available in Hex](https://hex.pm/docs/publish), the package can be installed
by adding `heavy` to your list of dependencies in `mix.exs`:

```elixir
def deps do
  [
    {:heavy, "~> 0.1.0"}
  ]
end
```

Documentation can be generated with [ExDoc](https://github.com/elixir-lang/ex_doc)
and published on [HexDocs](https://hexdocs.pm). Once published, the docs can
be found at [https://hexdocs.pm/heavy](https://hexdocs.pm/heavy).

