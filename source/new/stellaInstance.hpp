#pragma once

#include "../stellaInstanceBase.hpp"
#include <string>
#include <vector>
#include <jaffarCommon/exceptions.hpp>
#include <jaffarCommon/serializers/contiguous.hpp>
#include <jaffarCommon/deserializers/contiguous.hpp>

#include "renderFlag.hpp"
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

namespace stella
{

class EmuInstance : public EmuInstanceBase
{
 public:

 EmuInstance() : EmuInstanceBase()
 {
   _dummyAllocation = malloc(1024 * 64);
 }

 ~EmuInstance()
 {
  free(_dummyAllocation);
 }

  virtual void initialize() override
  {
    Settings::Options opts;
    _a2600 = MediaFactory::createOSystem();
    if(!_a2600->initialize(opts)) JAFFAR_THROW_RUNTIME("ERROR: Couldn't create A2600 System");

    #ifdef _JAFFAR_PLAYER
    _a2600->allocateFrameBuffer();
    _a2600->initializeVideo();
    #endif
  }

  virtual bool loadROMImpl(const std::string &romFilePath) override
  {
    const string romfile = romFilePath;
    const FSNode romnode(romfile);
 
    _a2600->createConsole(romnode);
    _ram = _a2600->console().riot().getRAM();
    _a2600->console().tia()._isTiaEnabled = true;
    
    return true;
  }

  void initializeVideoOutput() override
  {
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

    void* buffer = s.getOutputDataBuffer();
    if (buffer == nullptr) buffer = _dummyAllocation;
    gameState.getByteArray((uint8_t*)buffer, _stateSize);
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
    return _ram;
  }

  size_t getWorkRamSize() const
  {
    return 128;
  }

  void updateRenderer() override
  {
    _a2600->renderFrame();
  }

  inline size_t getDifferentialStateSizeImpl() const override { return getStateSizeImpl(); }

  void enableStateBlockImpl(const std::string& block) override
  {
    if (block == "TIA") { _a2600->console().tia()._isTiaEnabled = true; return; }

    JAFFAR_THROW_LOGIC("State block name: '%s' not found.", block.c_str());
  }

  void disableStateBlockImpl(const std::string& block) override
  {
    if (block == "TIA") { _a2600->console().tia()._isTiaEnabled = false; return; }

    JAFFAR_THROW_LOGIC("State block name: '%s' not found", block.c_str());
  }

  void doSoftReset() override
  {
    _a2600->console().system().reset();
  }
  
  void doHardReset() override
  {
  }

  std::string getCoreName() const override { return "Quicker Stella"; }


  void advanceStateImpl(stella::Controller controller) override
  {
    const auto controller1 = controller.getController1Code();

    if (controller1 & 0b00000001) _a2600->console().leftController().write(::Controller::DigitalPin::One,   false); else _a2600->console().leftController().write(::Controller::DigitalPin::One,   true);
    if (controller1 & 0b00000010) _a2600->console().leftController().write(::Controller::DigitalPin::Two,   false); else _a2600->console().leftController().write(::Controller::DigitalPin::Two,   true);
    if (controller1 & 0b00000100) _a2600->console().leftController().write(::Controller::DigitalPin::Three, false); else _a2600->console().leftController().write(::Controller::DigitalPin::Three, true);
    if (controller1 & 0b00001000) _a2600->console().leftController().write(::Controller::DigitalPin::Four,  false); else _a2600->console().leftController().write(::Controller::DigitalPin::Four,  true);
    if (controller1 & 0b00100000) _a2600->console().leftController().write(::Controller::DigitalPin::Six,   false); else _a2600->console().leftController().write(::Controller::DigitalPin::Six,   true);
    if (controller.getRightDifficultyState()) _a2600->console().switches().values() &= ~0x01; else _a2600->console().switches().values() |= 0x01;
    if (controller.getLeftDifficultyState())  _a2600->console().switches().values() &= ~0x40; else _a2600->console().switches().values() |= 0x40;

    auto tmp = stella::_renderingEnabled;
    stella::_renderingEnabled = false;
    _a2600->advanceFrame();
    stella::_renderingEnabled = tmp;
  }

  private:

  std::string _romData;
  std::unique_ptr<OSystem> _a2600;
  uint8_t* _ram;
  unique_ptr<OSystem> _theOSystem;
  void* _dummyAllocation;
};

} // namespace stella