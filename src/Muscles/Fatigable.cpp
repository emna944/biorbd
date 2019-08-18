#define BIORBD_API_EXPORTS
#include "Muscles/Fatigable.h"

#include "Utils/Error.h"
#include "Muscles/Muscle.h"
#include "Muscles/FatigueDynamicStateXia.h"

biorbd::muscles::Fatigable::Fatigable(const biorbd::utils::String &dynamicFatigueType)
{
    if (!dynamicFatigueType.tolower().compare("simple"))
        m_fatigueState = std::make_shared<biorbd::muscles::FatigueState>();
    else if (!dynamicFatigueType.tolower().compare("xia"))
        m_fatigueState = std::make_shared<biorbd::muscles::FatigueDynamicStateXia>();
    else
        biorbd::utils::Error::error(false, "Wrong muscle fatigue type");
}

biorbd::muscles::Fatigable::Fatigable(const biorbd::muscles::Muscle &m)
{
    try {
        const biorbd::muscles::Fatigable& m_tp(dynamic_cast<const biorbd::muscles::Fatigable&>(m));
        this->m_fatigueState = m_tp.m_fatigueState;
    } catch (const std::bad_cast&) {
        biorbd::utils::Error::error(false, "This muscle is not fatigable");
    }
}

biorbd::muscles::Fatigable::Fatigable(const std::shared_ptr<biorbd::muscles::Muscle> m)
{
    const std::shared_ptr<biorbd::muscles::Fatigable> m_tp(std::dynamic_pointer_cast<biorbd::muscles::Fatigable>(m));
    if (!m_tp)
        biorbd::utils::Error::error(false, "This muscle is not fatigable");
    this->m_fatigueState = m_tp->m_fatigueState;
}

biorbd::muscles::Fatigable::~Fatigable()
{

}

std::shared_ptr<biorbd::muscles::FatigueState> biorbd::muscles::Fatigable::fatigueState()
{
    return m_fatigueState;
}

void biorbd::muscles::Fatigable::fatigueState(double active, double fatigued, double resting)
{
    m_fatigueState->setState(active, fatigued, resting);
}


void biorbd::muscles::Fatigable::computeTimeDerivativeState(const biorbd::muscles::StateDynamics &emg)
{
    if (std::dynamic_pointer_cast<biorbd::muscles::FatigueDynamicState>(m_fatigueState)) {
        biorbd::muscles::Muscle* muscle = dynamic_cast<biorbd::muscles::Muscle*>(this);
        if (muscle)
            std::static_pointer_cast<biorbd::muscles::FatigueDynamicState>(m_fatigueState)->timeDerivativeState(emg, muscle->caract());
        else
            biorbd::utils::Error::error(false, "biorbd::muscles::Fatigable should be a biorbd::muscles::Muscle");
   } else {
       biorbd::utils::Error::error(false, "Type cannot be fatigued");
    }
}