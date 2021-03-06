//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#ifndef remus_proto_JobProgress_h
#define remus_proto_JobProgress_h

#include <string>
#include <sstream>

#include <remus/common/remusGlobals.h>
#include <remus/proto/conversionHelpers.h>

namespace remus {
namespace proto {

  //forward class declare for friend
  class JobStatus;

  //forward function declare for friend
  remus::proto::JobStatus to_JobStatus(const std::string& status);
}

}


namespace remus {
namespace proto {

//Job progress is a helper class to easily state what the progress of a currently
//running job is. Progress can be numeric, textual or both.
struct JobProgress
{
  JobProgress():
    Value(-1),
    Message()
    {}

  explicit JobProgress(remus::STATUS_TYPE status):
    Value(-1),
    Message()
    {
    if(status==remus::IN_PROGRESS)
      {
      //if this is a valid in_progress job progress struct, than make
      //value 0
      this->Value = 0;
      }
    }

  explicit JobProgress(int v):
    Value(valid_progress_value(v)),
    Message()
    {}

  explicit JobProgress(const std::string& msg):
    Value(0),
    Message(msg)
    {}

  JobProgress(int v, const std::string& msg):
    Value(valid_progress_value(v)),
    Message(msg)
    {}

  //overload on the progress object to make it easier to detect when
  //progress has been changed.
  bool operator ==(const JobProgress& b) const
  {
    return this->Value == b.Value && this->Message == b.Message;
  }

  //overload on the progress object to make it easier to detect when
  //progress has been changed.
  bool operator !=(const JobProgress& b) const
  {
    return !(this->operator ==(b));
  }

  int value() const { return Value; }
  const std::string message() const { return Message; }

  void setValue(int v) { Value = JobProgress::valid_progress_value(v); }
  void setMessage(const std::string& msg) { Message = msg; }


  //make sure that we can't set progress to be outside the valid range.
  static inline int valid_progress_value(int v)
  {
    v = std::min<int>(v,100);
    v = std::max<int>(v,1);
    return v;
  }

  //make it easy to print out the progress, by overloading the << operator
  //when using cout, cerr, etc
  friend std::ostream& operator<< (std::ostream& out, const JobProgress& j)
  {
    if(j.Value < 0)
      {
      out << "Progress: Invalid";
      }
    else if(j.Value > 0 && j.Message.size() > 0)
      {
      out << "Progress[" << j.Value << "%]: " << j.Message;
      }
    else if(j.Value > 0)
      {
      out << "Progress: " << j.Value << "%";
      }
    else
      {
      out << "Progress: " << j.Message;
      }
    return out;
  }

private:
  int Value;
  std::string Message;
};

}
}
#endif
