#pragma once
#include "Handlers/IProfilePreprocessor.hpp"

class InternalProfilePreprocessor : public Qv2rayPlugin::Profile::IProfilePreprocessor
{
  public:
    virtual ~InternalProfilePreprocessor() = default;
    virtual ProfileContent PreprocessProfile(const ProfileContent &) override;
};
