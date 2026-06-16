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

#ifndef TIA_DELAY_QUEUE
#define TIA_DELAY_QUEUE

#include "Serializable.hxx"
#include "bspf.hxx"
#include "smartmod.hxx"
#include "DelayQueueMember.hxx"

template<unsigned length, unsigned capacity>
class DelayQueueIteratorImpl;

template<unsigned length, unsigned capacity>
class DelayQueue : public Serializable
{
  public:
    friend DelayQueueIteratorImpl<length, capacity>;

  public:
    DelayQueue();

  public:

    void push(uInt8 address, uInt8 value, uInt8 delay);

    void reset();

    template<typename T> void execute(T executor);

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

  private:
    std::array<DelayQueueMember<capacity>, length> myMembers;
    uInt8 myIndex{0};
    std::array<uInt8, 0xFF> myIndices;

    // Total number of entries scheduled across all members. Lets execute()
    // take a fast path (just advance the index) on the very common color
    // clocks where nothing is pending. Derived state: not serialized, it is
    // recomputed in load().
    uInt32 myPendingCount{0};

  private:
    DelayQueue(const DelayQueue&) = delete;
    DelayQueue(DelayQueue&&) = delete;
    DelayQueue& operator=(const DelayQueue&) = delete;
    DelayQueue& operator=(DelayQueue&&) = delete;
};

// ############################################################################
// Implementation
// ############################################################################

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
DelayQueue<length, capacity>::DelayQueue()
{
  myIndices.fill(0xFF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
void DelayQueue<length, capacity>::push(uInt8 address, uInt8 value, uInt8 delay)
{
  if (delay >= length)
    throw runtime_error("delay exceeds queue length");

  const uInt8 currentIndex = myIndices[address];

  if (currentIndex < length)
  {
    myMembers[currentIndex].remove(address);
    --myPendingCount;
  }

  const uInt8 index = smartmod<length>(myIndex + delay);
  myMembers[index].push(address, value);
  ++myPendingCount;

  myIndices[address] = index;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
void DelayQueue<length, capacity>::reset()
{
  for (uInt32 i = 0; i < length; ++i)
    myMembers[i].clear();

  myIndex = 0;
  myPendingCount = 0;
  myIndices.fill(0xFF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
template<typename T>
void DelayQueue<length, capacity>::execute(T executor)
{
  // Fast path: nothing is scheduled anywhere in the queue, which is the case
  // on the vast majority of color clocks. Skip touching the member array
  // entirely and just advance the index.
  if (myPendingCount != 0)
  {
    DelayQueueMember<capacity>& currentMember = myMembers[myIndex];

    for (uInt8 i = 0; i < currentMember.mySize; ++i) {
      executor(currentMember.myEntries[i].address, currentMember.myEntries[i].value);
      myIndices[currentMember.myEntries[i].address] = 0xFF;
    }

    myPendingCount -= currentMember.mySize;
    currentMember.clear();
  }

  myIndex = smartmod<length>(myIndex + 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
bool DelayQueue<length, capacity>::save(Serializer& out) const
{
  try
  {
    out.putInt(length);

    for (uInt32 i = 0; i < length; ++i)
      myMembers[i].save(out);

    out.putByte(myIndex);
    // myIndices is the reverse map (address -> member) of the members above,
    // so it is derived state and recomputed on load instead of serialized.
  }
  catch(...)
  {
    cerr << "ERROR: TIA_DelayQueue::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
bool DelayQueue<length, capacity>::load(Serializer& in)
{
  try
  {
    if (in.getInt() != length) throw runtime_error("delay queue length mismatch");

    for (uInt32 i = 0; i < length; ++i)
      myMembers[i].load(in);

    myIndex = in.getByte();

    // Rebuild derived state (the address -> member reverse map and the pending
    // entry count) from the restored members.
    myIndices.fill(0xFF);
    myPendingCount = 0;
    for (uInt32 i = 0; i < length; ++i)
    {
      const DelayQueueMember<capacity>& member = myMembers[i];
      myPendingCount += member.mySize;
      for (uInt8 j = 0; j < member.mySize; ++j)
        myIndices[member.myEntries[j].address] = static_cast<uInt8>(i);
    }
  }
  catch(...)
  {
    cerr << "ERROR: TIA_DelayQueue::load" << endl;
    return false;
  }

  return true;
}

#endif //  TIA_DELAY_QUEUE
