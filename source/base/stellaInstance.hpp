#pragma once

#include "../stellaInstanceBase.hpp"
#include <string>
#include <vector>
#include <jaffarCommon/exceptions.hpp>
#include <jaffarCommon/serializers/contiguous.hpp>
#include <jaffarCommon/deserializers/contiguous.hpp>

#include "EventHandlerSDL2.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "MediaFactory.hxx"
#include "Serializer.hxx"
#include "StateManager.hxx"
#include "Console.hxx"
#include "Control.hxx"
#include "Switches.hxx"
#include "M6532.hxx"
#include "TIA.hxx"
#include "renderFlag.hpp"

namespace stella
{

class EmuInstance : public EmuInstanceBase
{
 public:

 EmuInstance(const nlohmann::json &config) : EmuInstanceBase(config)
 {
  Settings::Options opts;
  _a2600 = MediaFactory::createOSystem();
  if(!_a2600->initialize(opts)) JAFFAR_THROW_RUNTIME("ERROR: Couldn't create A2600 System");
 }

 ~EmuInstance()
 {
 }

  virtual void initialize() override
  {
  }

  virtual bool loadROMImpl(const std::string &romFilePath) override
  {
    const string romfile = romFilePath;
    const FSNode romnode(romfile);

    _a2600->createConsole(romnode);
    _ram = _a2600->console().riot().getRAM();
    
    return true;
  }

  void initializeVideoOutput() override
  {
    _a2600->initializeVideo();
  }

  void finalizeVideoOutput() override
  {
  }

  void enableRendering() override
  {
    stella::_renderingEnabled = true;
  }

  void disableRendering() override
  {
    stella::_renderingEnabled = false;
  }

  void serializeState(jaffarCommon::serializer::Base& s) const override
  {
    Serializer gameState;
    _a2600->state().saveState(gameState);
    gameState.getByteArray(s.getOutputDataBuffer(), _stateSize);
    s.pushContiguous(nullptr, _stateSize);
  }

  void deserializeState(jaffarCommon::deserializer::Base& d) override
  {
    Serializer gameState;
    gameState.putByteArray(d.getInputDataBuffer(), _stateSize);
    _a2600->state().loadState(gameState);
    d.popContiguous(nullptr, _stateSize);
  }

  size_t getStateSizeImpl() const override
  {
    Serializer gameState;
    _a2600->state().saveState(gameState);
    return gameState.size();
  }

  uint8_t* getWorkRamPointer() const override
  {
    return (uint8_t*)(uint64_t)_ram;
  }

  void updateRenderer() override
  {
    _a2600->renderFrame();
  }

  inline size_t getDifferentialStateSizeImpl() const override { return getStateSizeImpl(); }

  void enableStateBlockImpl(const std::string& block) override
  {
    JAFFAR_THROW_LOGIC("State block name: '%s' not found.", block.c_str());
  }

  void disableStateBlockImpl(const std::string& block) override
  {
    JAFFAR_THROW_LOGIC("State block name: '%s' not found", block.c_str());
  }

  void doSoftReset() override
  {
    _a2600->console().system().reset();
  }
  
  void doHardReset() override
  {
  }

  std::string getCoreName() const override { return "Stella"; }


  void advanceStateImpl(const jaffar::input_t &input) override
  {
    if (input.port1 & 0b00000001) _a2600->console().leftController().write(::Controller::DigitalPin::One,   false); else _a2600->console().leftController().write(::Controller::DigitalPin::One,   true);
    if (input.port1 & 0b00000010) _a2600->console().leftController().write(::Controller::DigitalPin::Two,   false); else _a2600->console().leftController().write(::Controller::DigitalPin::Two,   true);
    if (input.port1 & 0b00000100) _a2600->console().leftController().write(::Controller::DigitalPin::Three, false); else _a2600->console().leftController().write(::Controller::DigitalPin::Three, true);
    if (input.port1 & 0b00001000) _a2600->console().leftController().write(::Controller::DigitalPin::Four,  false); else _a2600->console().leftController().write(::Controller::DigitalPin::Four,  true);
    if (input.port1 & 0b00100000) _a2600->console().leftController().write(::Controller::DigitalPin::Six,   false); else _a2600->console().leftController().write(::Controller::DigitalPin::Six,   true);
    if (input.rightDifficulty) _a2600->console().switches().values() &= ~0x01; else _a2600->console().switches().values() |= 0x01;
    if (input.leftDifficulty)  _a2600->console().switches().values() &= ~0x40; else _a2600->console().switches().values() |= 0x40;

    _a2600->advanceFrame();
  }

  private:

  std::string _romData;
  std::unique_ptr<OSystem> _a2600;
  const uint8_t* _ram;
  unique_ptr<OSystem> _theOSystem;
};

} // namespace stella