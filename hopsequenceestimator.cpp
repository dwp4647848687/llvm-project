#include <algorithm>
#include <numeric>
#include <map>
#include "hopsequenceestimator.hpp"
#include "libDragonLink/include/hopsequencemaker.hpp"

namespace md1::dragonlink {
//------1-------2-------3-------4-------5-------6-------7-------8-------9-------10------11------12------13------14------15------

HopSequenceEstimator::HopSequenceEstimator(std::array<uint64_t, DragonLinkParams::NUM_HOPS>& channelFrequencies) :
    m_channelFrequencies(channelFrequencies),
    m_currentChannelIndex(0)
{
    m_packets.resize(DragonLinkParams::NUM_HOPS) ;
}

void HopSequenceEstimator::reset()
{
    m_currentChannelIndex = 0 ;
    m_packets.clear() ;
    m_packets.resize(DragonLinkParams::NUM_HOPS) ;
}

bool HopSequenceEstimator::insertPacket(Packet& packet)
{
    bool retval = true ;
    m_packets[m_currentChannelIndex].push_back(packet) ;
    return retval ;
}

uint64_t HopSequenceEstimator::getCurrentChannelFrequency() const
{
    return m_channelFrequencies[m_currentChannelIndex] ;
}

void HopSequenceEstimator::next()
{
    m_currentChannelIndex++ ;
    
    if (m_currentChannelIndex >= m_packets.size())
    {
        m_currentChannelIndex = 0 ;
    }
}

bool HopSequenceEstimator::estimateAvailable(EstimationParameters& params, HopSequence& sequence) const
{
    // scan through the packets we have received. What we need is to be able to get servo positions for all channels
    // recall that channels 5,6,7,8 are multiplexed with other transmit power and some other data so we need enough packets
    // to let us get our hands on all servo positions.
    bool retval = false ;

    std::vector<uint32_t> linkIds ;
    uint32_t linkid_occurrances            = 0 ;
    uint32_t linkid_mode                   = 0 ;
    std::vector<uint32_t> txPower ;
    PacketMaker::TransmitPower txPowerMode = PacketMaker::TransmitPower::PWR_25MW ;
    uint32_t txPower_occurrances           = 0 ;
    std::vector<std::vector<uint16_t>> channelData(DragonLinkParams::DEFAULT_NUM_CHANNELS) ;
    ChannelData channelAverages ;
    std::vector< std::vector<uint8_t> > hopIds(DragonLinkParams::NUM_HOPS) ;

    // documenting the variables being used the estimation algorithm
    // std vector linkIds -> a vector of all the link ids foudn in the packets
    // uint32_t linkMode -> the mode of the link ids
    // uint32_t linkid_occurrances -> the number of times the mode link id occurs
    // vector of vectors channelData -> for those packets having the linkMode link id the channel data is stored here
    // ChannelData channelAverages -> for each of the channels in the payload the average value is stored here
    
    // std vector txPower           -> a vector of all tx power founds in the packets
    // uint32_t txPowerMode         -> the mode of the tx power values
    // uint32_t txPower_occurrances -> the number of times the mode tx power value occurs
    // vector of vectors -> hopIds for each packet

    // Find the LinkID mode and we will use this as the link for everything going forward
    getLinkIds(linkIds) ;
    linkid_mode = findMode(linkIds, linkid_occurrances) ;

    // we are now going to:
    // 1. find the average servo positions for each channel
    // 2. create the table of hops ids
    // 3. create the list of transmit powers
    //if ((linkid_mode != 0) && (linkid_occurrances >= 3))    // mode can't be zero and we must have at least 3 packets with the same link id
    if (linkid_mode != 0)    // mode can't be zero and we must have at least 3 packets with the same link id
    {
        // we will scan through all packets and find the ones with the correct link id. For each packet we then extract the channel data and store it
        for (size_t rows = 0 ; rows < m_packets.size() ; rows++)
        {
            for (size_t cols = 0 ; cols < m_packets[rows].size() ; cols++)
            {
                if (m_packets[rows][cols].m_syncWord.getLinkId() == linkid_mode)
                {
                    // look at the payload header byte and see what type it is
                    PacketMaker::Override override = static_cast<PacketMaker::Override>(m_packets[rows][cols].m_payload.getHeaderByte() & 0x0F);
                    uint8_t override_channel = PacketMaker::OverrideToChannel(static_cast<uint8_t>(override));

                    for (size_t i = 0 ; i < m_packets[rows][cols].m_payload.numChannels() ; i++)   // iterate over each channel in the packet
                    {
                        if (i != static_cast<size_t>(override_channel))
                        {
                            uint16_t value = 0 ;
                            m_packets[rows][cols].m_payload.getChannel(i, value) ;
                            channelData[i].push_back(value) ;
                        }
                        else if (override_channel == 4 || override_channel == 6)
                        {
                            uint16_t value = 0 ;
                            m_packets[rows][cols].m_payload.getChannel(i, value) ;
                            txPower.push_back(static_cast<uint32_t>(value));
                        }
                    }

                    hopIds[rows].push_back(m_packets[rows][cols].m_syncWord.getHopSequenceNumber()) ;
                }
            }
        }

        // find the mode of the tx power
        txPowerMode = static_cast<PacketMaker::TransmitPower>(findMode(txPower, txPower_occurrances)) ;

        // to get here we should have all the channel data for all channels ... check that the length of all vectors in the channelData vector are not zero
        bool allChannelsDataAvailable = std::all_of(channelData.begin(), channelData.end(), [](const std::vector<uint16_t>& v) { return v.size() > 0 ; }) ;

        if (allChannelsDataAvailable == true)
        {
            size_t index = 0 ;
            for (const auto& channel : channelData)
            {
                uint32_t sum = std::accumulate(channel.begin(), channel.end(), 0) ;
                uint32_t avg = sum / channel.size() ;
                channelAverages.m_value[index++] = avg ;
            }

            // we have all of the data we need to make an estimate ... so form the hop sequence
            HopSequence hopSequence ;
            HopSequenceMaker hopSequenceMaker ;
            // we now have a hop sequence, however the hop sequence maker will make a sequence always starting at the lowest hop going upwards.
            // however the hop sequence we have been provided won't start at the lowest hop so we need to re-order the sequence generated to start
            // at the same hop sequence as that provided to us.
            uint8_t firstHop = 0 ;
            size_t  firstHopIndex = 0 ;

            for ( size_t r = 0 ; r < hopIds.size() ; r++ )
            {
                if ( hopIds[r].size() != 0 )
                {
                    firstHop = hopIds[r][0] ;
                    firstHopIndex = r ;
                    break ;
                }
            }

            int num_hops = static_cast<int>( DragonLinkParams::NUM_HOPS ) ;
            uint8_t calculatedFirstHopIndex = static_cast<uint8_t>((static_cast<int>(firstHop) - static_cast<int>(firstHopIndex) + num_hops) % num_hops) ;

            hopSequenceMaker( static_cast<uint16_t>(linkid_mode), calculatedFirstHopIndex, txPowerMode, channelAverages, hopSequence) ;
            
            params.m_linkIdMode           = linkid_mode ;
            params.m_linkIdOccurrances    = linkid_occurrances ;
            params.m_txPowerMode          = txPowerMode ;
            params.m_txPowerOccurrances   = txPower_occurrances ;
            params.m_channelAverages      = channelAverages ;
            params.m_firstHopId           = static_cast<uint32_t>(firstHop) ;
            params.m_firstHopIdIndex      = static_cast<uint32_t>(firstHopIndex);
            params.m_calculatedFirstHopId = static_cast<uint32_t>(calculatedFirstHopIndex) ;
            sequence = hopSequence ;
            retval = true ;
        }
    }
    else
    {
        // we have not received enough packets to make an estimate
    }

    
    // for all the packets we have received are the IDs the same?
    return retval ;
}

void HopSequenceEstimator::getLinkIds(std::vector<uint32_t>& linkIds) const
{
    linkIds.clear() ;
    for (const auto& packet : m_packets)
    {
        for (const auto& packet : packet)
        {
            linkIds.push_back(packet.m_syncWord.getLinkId()) ;
        }
    }
}

uint32_t HopSequenceEstimator::findMode(std::vector<uint32_t>& data, uint32_t& occurrances) const
{
    // find the mode of the data
    uint32_t mode = 0 ;
    std::map<uint32_t, uint32_t> counts ;
    for (const auto& d : data)
    {
        counts[d]++ ;
    }

    uint32_t maxCount = 0 ;
    for (const auto& c : counts)
    {
        if (c.second > maxCount)
        {
            maxCount = c.second ;
            mode = c.first ;
        }
    }

    occurrances = maxCount ;
    return mode ;
}

//------1-------2-------3-------4-------5-------6-------7-------8-------9-------10------11------12------13------14------15------
}  // namespace md1::dragonlink
