module Main where

import Data.Bits(testBit, (.&.), shiftR)

type Pin = Int

data ProtocolElement = ProtoPin Pin | ProtoComment String

data DHTNumber = DHTNumber Int Int

instance Show DHTNumber where
  show (DHTNumber a b) = show a <> "." <> show b

instance Show ProtocolElement where
  show (ProtoPin p) = show p
  show (ProtoComment c) = "# " <> c

type Protocol = [ProtocolElement]

numberBits8 :: Int -> [Bool]
numberBits8 a = reverse (testBit a <$> [0..7])

dhtToBytes :: DHTNumber -> [Int]
dhtToBytes (DHTNumber a b) =
  let result = a*10+b
  in [result `shiftR` 8, result .&. 0xff]

encodeBytes :: [Int] -> Protocol
encodeBytes xs = concatMap (\(i, b) -> [ProtoComment ("byte " <> show i <> ": " <> show b)] <> (encodeBits (numberBits8 b))) ([1..] `zip` xs)

generateSensor :: DHTNumber -> DHTNumber -> Protocol
generateSensor humid temp =
  let humidBytes = dhtToBytes humid
      tempBytes = dhtToBytes temp
  in [ProtoComment ("encode humidity " <> show humid <> " begin")]
  <> encodeBytes humidBytes
  <> [ProtoComment ("encode humidity end")]
  <> [ProtoComment ("encode temp " <> show temp <> " begin")]
  <> encodeBytes tempBytes
  <> [ProtoComment ("encode temp end")]
  <> [ProtoComment "encode checksum"]
  <> encodeBytes [sum (humidBytes <> tempBytes) .&. 0xff]
  <> [ProtoComment ("encode checksum end")]

encodeBits :: [Bool] -> Protocol
encodeBits x = concatMap encodeBit (zip [0..] x)
  where encodeBit (i, True) = [ProtoComment ("bit " <> show i)] <> (ProtoPin <$> [sensorPin, sensorPin, 0])
        encodeBit (i, False) = [ProtoComment ("bit " <> show i)] <> (ProtoPin <$> [0, sensorPin, 0])

outputProtocol :: Protocol -> String
outputProtocol = unlines . fmap show

sensorPin :: Pin
sensorPin = 13

sensorReadStart :: Protocol
sensorReadStart = [ProtoComment "start read codon start", ProtoPin sensorPin, ProtoPin 0, ProtoComment "start read codon end"]

main = putStr (outputProtocol (sensorReadStart <> generateSensor (DHTNumber 34 5) (DHTNumber 67 8)))
  
