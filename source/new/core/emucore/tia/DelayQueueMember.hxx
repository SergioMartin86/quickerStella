//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2023 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef TIA_DELAY_QUEUE_MEMBER
#define TIA_DELAY_QUEUE_MEMBER

#include "bspf.hxx"
#include "Serializable.hxx"

template<unsigned capacity>
class DelayQueueMember : public Serializable {

  public:
    struct Entry {
      uInt8 address{0};
      uInt8 value{0};
    };

  public:
    DelayQueueMember();

  public:
    void push(uInt8 address, uInt8 value);

    void remove(uInt8 address);

    void clear();

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

  public:
    std::array<Entry, capacity> myEntries;
    uInt8 mySize{0};

  private:
    DelayQueueMember(const DelayQueueMember<capacity>&) = delete;
    DelayQueueMember(DelayQueueMember<capacity>&&) = delete;
    DelayQueueMember<capacity>& operator=(const DelayQueueMember<capacity>&) = delete;
    DelayQueueMember<capacity>& operator=(DelayQueueMember<capacity>&&) = delete;

};

// ############################################################################
// Implementation
// ############################################################################

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned capacity>
DelayQueueMember<capacity>::DelayQueueMember()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned capacity>
void DelayQueueMember<capacity>::push(uInt8 address, uInt8 value)
{
  if (mySize == capacity) throw runtime_error("delay queue overflow");

  myEntries[mySize].address = address;
  myEntries[mySize++].value = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned capacity>
void DelayQueueMember<capacity>::remove(uInt8 address)
{
  uInt8 index;

  for (index = 0; index < mySize; ++index) {
    if (myEntries[index].address == address) break;
  }

  if (index < mySize) {
    for (uInt8 i = index + 1; i < mySize; ++i) {
      myEntries[i-1] = myEntries[i];
    }

    --mySize;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned capacity>
void DelayQueueMember<capacity>::clear()
{
  mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned capacity>
bool DelayQueueMember<capacity>::save(Serializer& out) const
{
  try
  {
    // Serialize a fixed `capacity` entries (zero-padding the unused slots) so
    // that the serialized size is constant regardless of how many entries are
    // pending. A variable size here makes the whole emulator state size vary,
    // which silently truncates/corrupts fixed-size state buffers on save/load.
    out.putByte(mySize);
    for(uInt32 i = 0; i < capacity; ++i)
    {
      const bool used = i < mySize;
      out.putByte(used ? myEntries[i].address : 0);
      out.putByte(used ? myEntries[i].value : 0);
    }
  }
  catch(...)
  {
    cerr << "ERROR: TIA_DelayQueueMember::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned capacity>
bool DelayQueueMember<capacity>::load(Serializer& in)
{
  try
  {
    mySize = in.getByte();
    if (mySize > capacity) throw runtime_error("invalid delay queue size");
    for(uInt32 i = 0; i < capacity; ++i)
    {
      const uInt8 address = in.getByte();
      const uInt8 value = in.getByte();
      if (i < mySize)
      {
        myEntries[i].address = address;
        myEntries[i].value = value;
      }
    }
  }
  catch(...)
  {
    cerr << "ERROR: TIA_DelayQueueMember::load" << endl;
    return false;
  }

  return true;
}

#endif // TIA_DELAY_QUEUE_MEMBER
