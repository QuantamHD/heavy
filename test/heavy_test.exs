defmodule HeavyTest do
  use ExUnit.Case
  doctest Heavy

  test "greets the world" do
    assert Heavy.hello() == :world
  end
end
