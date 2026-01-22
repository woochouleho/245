/*
 * sockmark_define.h --- header file for socket mark value define
 */

/*QoS definitions, and pass these information to linux kernel

 * wan 802.1p mark 0-2 bit: 802.1p
 * We can not use 0 to as tc handle. So we use (i+1).
 *======================================================================
 *
 * For port based/protocol based/Source IP based ....etc VLAN function.
 *
 * Using Mark to let driver know how to insert 802.1Q header.
 * The rule is as following design.
 * Meter index         [31:27](5 bits): SW/HW meter index
 * Meter index enabled [26:26](1 bits): SW/HW meter enable
 * Reserved            [25:21](5 bits): Reserved
 * From LAN		       [20:20](1 bits): Upstream from LAN
 * MAPE local route	   [19:19](1 bits): MAPE local route
 * WAN Mark 	       [18:13](6 bits): WAN MARK used for policy route
 * Qos Entry	[12:7]( 6 bits): Qos Entry ID
 * SW QID		[6:4]( 4 bits): Queue ID (0~7)
 * 1p Enable	       [3:3](1 bit): 802.1p remarking enable
 * 802.1p		[2:0]( 3 bits): 802.1p value. (0~7)
 *
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Meter index [27:31] |Meter enable [26:26]|Reserved [25:21]|From LAN|MAPE local route|WAN Mark|QosEntryID|SWQID|e| p   |
 * |       (5bits)       |      (1bits)       |     5bits      | 1bits  |      1bits     |  6bits |[12:7](6b)|[6:4]|1|[2:0]|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *======================================================================*/


#ifndef INCLUDE_SOCKMARKDEFINE_H
#define INCLUDE_SOCKMARKDEFINE_H
/* 802.1p remarking: mark[2:0] */
#define SOCK_MARK_8021P_START                                       0
#define SOCK_MARK_8021P_END                                         2
#define SOCK_MARK_8021P_BIT_NUM                                     (SOCK_MARK_8021P_END-SOCK_MARK_8021P_START+1)
#define SOCK_MARK_8021P_BIT_MASK                                    (((1<<SOCK_MARK_8021P_BIT_NUM)-1) << SOCK_MARK_8021P_START)
#define SOCK_MARK_MARK_TO_8021P(MARK)                               ((MARK & SOCK_MARK_8021P_BIT_MASK) >> SOCK_MARK_8021P_START)
#define SOCK_MARK_8021P_TO_MARK(PRI_VAL)                            (PRI_VAL << SOCK_MARK_8021P_START)

/* 802.1p remarking enable: mark[3:3] */
#define SOCK_MARK_8021P_ENABLE_START                                3
#define SOCK_MARK_8021P_ENABLE_END                                  3
#define SOCK_MARK_8021P_ENABLE_BIT_NUM                              (SOCK_MARK_8021P_ENABLE_END-SOCK_MARK_8021P_ENABLE_START+1)
#define SOCK_MARK_8021P_ENABLE_BIT_MASK                             (((1<<SOCK_MARK_8021P_ENABLE_BIT_NUM)-1) << SOCK_MARK_8021P_ENABLE_START)
#define SOCK_MARK_MARK_TO_8021P_ENABLE(MARK)                        ((MARK & SOCK_MARK_8021P_ENABLE_BIT_MASK) >> SOCK_MARK_8021P_ENABLE_START)
#define SOCK_MARK_8021P_ENABLE_TO_MARK(PRI_ENABLE)                  (PRI_ENABLE << SOCK_MARK_8021P_ENABLE_START)

/* QoS queue ID: mark[6:4] */
#define SOCK_MARK_QOS_SWQID_START                                   4
#define SOCK_MARK_QOS_SWQID_END                                     6
#define SOCK_MARK_QOS_SWQID_BIT_NUM                                 (SOCK_MARK_QOS_SWQID_END-SOCK_MARK_QOS_SWQID_START+1)
#define SOCK_MARK_QOS_SWQID_BIT_MASK                                (((1<<SOCK_MARK_QOS_SWQID_BIT_NUM)-1) << SOCK_MARK_QOS_SWQID_START)
#define SOCK_MARK_MARK_TO_QOS_SWQID_ENABLE(MARK)                    ((MARK & SOCK_MARK_QOS_SWQID_BIT_MASK) >> SOCK_MARK_QOS_SWQID_START)
#define SOCK_MARK_QOS_SWQID_TO_MARK(QOS_SWQID)                      (QOS_SWQID << SOCK_MARK_QOS_SWQID_START)

/* QoS entry: mark[12:7] */
#define SOCK_MARK_QOS_ENTRY_START                                   7
#define SOCK_MARK_QOS_ENTRY_END                                     12
#define SOCK_MARK_QOS_ENTRY_BIT_NUM                                 (SOCK_MARK_QOS_ENTRY_END-SOCK_MARK_QOS_ENTRY_START+1)
#define SOCK_MARK_QOS_ENTRY_BIT_MASK                                (((1<<SOCK_MARK_QOS_ENTRY_BIT_NUM)-1) << SOCK_MARK_QOS_ENTRY_START)
#define SOCK_MARK_MARK_TO_QOS_ENTRY_INDEX(MARK)                     ((MARK & SOCK_MARK_QOS_ENTRY_BIT_MASK) >> SOCK_MARK_QOS_ENTRY_START)
#define SOCK_MARK_QOS_ENTRY_TO_MARK(QOS_ENTRY)                      (QOS_ENTRY << SOCK_MARK_QOS_ENTRY_START)

/* WAN index for policy route: mark[18:13] */
#define SOCK_MARK_WAN_INDEX_START                                   13
#define SOCK_MARK_WAN_INDEX_END                                     18
#define SOCK_MARK_WAN_INDEX_BIT_NUM                                 (SOCK_MARK_WAN_INDEX_END-SOCK_MARK_WAN_INDEX_START+1)
#define SOCK_MARK_WAN_INDEX_BIT_MASK                                (((1<<SOCK_MARK_WAN_INDEX_BIT_NUM)-1) << SOCK_MARK_WAN_INDEX_START)
#define SOCK_MARK_MARK_TO_WAN_INDEX(MARK)                           ((MARK & SOCK_MARK_WAN_INDEX_BIT_MASK) >> SOCK_MARK_WAN_INDEX_START)
#define SOCK_MARK_WAN_INDEX_TO_MARK(WAN_INDEX)                      (WAN_INDEX << SOCK_MARK_WAN_INDEX_START)
#if 0
#define SOCK_MARK_MARK_TO_WAN_VPN_PPTP_INDEX(MARK)                  (((MARK & SOCK_MARK_WAN_INDEX_BIT_MASK)-SOCK_MARK_MARK_TO_WAN_INDEX(SOCK_MARK_WAN_INDEX_BIT_MASK - ((MAX_PPTP_NUM+MAX_L2TP_NUM) << SOCK_MARK_WAN_INDEX_START))) >> SOCK_MARK_WAN_INDEX_START)
#define SOCK_MARK_MARK_TO_WAN_VPN_L2TP_INDEX(MARK)                  (((MARK & SOCK_MARK_WAN_INDEX_BIT_MASK)-SOCK_MARK_MARK_TO_WAN_INDEX((SOCK_MARK_WAN_INDEX_BIT_MASK - (MAX_L2TP_NUM << SOCK_MARK_WAN_INDEX_START)))) >> SOCK_MARK_WAN_INDEX_START)
#define SOCK_MARK_MARK_TO_WAN_VXLAN_INDEX(MARK)                     (((MARK & SOCK_MARK_WAN_INDEX_BIT_MASK)-SOCK_MARK_MARK_TO_WAN_INDEX((SOCK_MARK_WAN_INDEX_BIT_MASK - ((MAX_PPTP_NUM+MAX_L2TP_NUM+MAX_VXLAN_NUM) << SOCK_MARK_WAN_INDEX_START)))) >> SOCK_MARK_WAN_INDEX_START)
#define SOCK_MARK_MARK_TO_WAN_PPPOA_INDEX(MARK)                     (((MARK & SOCK_MARK_WAN_INDEX_BIT_MASK)-SOCK_MARK_MARK_TO_WAN_INDEX((SOCK_MARK_WAN_INDEX_BIT_MASK - ((MAX_PPTP_NUM+MAX_L2TP_NUM+MAX_VXLAN_NUM+MAX_PPPOA_NUM) << SOCK_MARK_WAN_INDEX_START)))) >> SOCK_MARK_WAN_INDEX_START)
#define SOCK_MARK_MARK_TO_WAN_PPPOE_INDEX(MARK)                     (((MARK & SOCK_MARK_WAN_INDEX_BIT_MASK)-SOCK_MARK_MARK_TO_WAN_INDEX((SOCK_MARK_WAN_INDEX_BIT_MASK - ((MAX_PPTP_NUM+MAX_L2TP_NUM+MAX_VXLAN_NUM+MAX_PPPOA_NUM+MAX_PPPOE_NUM) << SOCK_MARK_WAN_INDEX_START)))) >> SOCK_MARK_WAN_INDEX_START)
#define SOCK_MARK_MARK_TO_WAN_ATM_INDEX(MARK)                       (((MARK & SOCK_MARK_WAN_INDEX_BIT_MASK)-SOCK_MARK_MARK_TO_WAN_INDEX((SOCK_MARK_WAN_INDEX_BIT_MASK - ((MAX_PPTP_NUM+MAX_L2TP_NUM+MAX_VXLAN_NUM+MAX_PPPOA_NUM+MAX_PPPOE_NUM+MAX_ATM_NUM) << SOCK_MARK_WAN_INDEX_START)))) >> SOCK_MARK_WAN_INDEX_START)
#define SOCK_MARK_MARK_TO_WAN_PTM_INDEX(MARK)                       (((MARK & SOCK_MARK_WAN_INDEX_BIT_MASK)-SOCK_MARK_MARK_TO_WAN_INDEX((SOCK_MARK_WAN_INDEX_BIT_MASK - ((MAX_PPTP_NUM+MAX_L2TP_NUM+MAX_VXLAN_NUM+MAX_PPPOA_NUM+MAX_PPPOE_NUM+MAX_ATM_NUM+MAX_PTM_NUM) << SOCK_MARK_WAN_INDEX_START)))) >> SOCK_MARK_WAN_INDEX_START)
#define SOCK_MARK_MARK_TO_WAN_ETH_INDEX(MARK)                       (((MARK & SOCK_MARK_WAN_INDEX_BIT_MASK)-SOCK_MARK_MARK_TO_WAN_INDEX((SOCK_MARK_WAN_INDEX_BIT_MASK - ((MAX_PPTP_NUM+MAX_L2TP_NUM+MAX_VXLAN_NUM+MAX_PPPOA_NUM+MAX_PPPOE_NUM+MAX_ATM_NUM+MAX_PTM_NUM+MAX_EWAN_NUM) << SOCK_MARK_WAN_INDEX_START)))) >> SOCK_MARK_WAN_INDEX_START)
#define SOCK_MARK_WAN_VPN_PPTP_INDEX_TO_MARK(WAN_INDEX)             ((WAN_INDEX << SOCK_MARK_WAN_INDEX_START)+(SOCK_MARK_WAN_INDEX_BIT_MASK - ((MAX_PPTP_NUM+MAX_L2TP_NUM) << SOCK_MARK_WAN_INDEX_START)))
#define SOCK_MARK_WAN_VPN_L2TP_INDEX_TO_MARK(WAN_INDEX)             ((WAN_INDEX << SOCK_MARK_WAN_INDEX_START)+(SOCK_MARK_WAN_INDEX_BIT_MASK - (MAX_L2TP_NUM << SOCK_MARK_WAN_INDEX_START)))
#define SOCK_MARK_WAN_VXLAN_INDEX_TO_MARK(WAN_INDEX)                ((WAN_INDEX << SOCK_MARK_WAN_INDEX_START)+(SOCK_MARK_WAN_INDEX_BIT_MASK - ((MAX_PPTP_NUM+MAX_L2TP_NUM+MAX_VXLAN_NUM) << SOCK_MARK_WAN_INDEX_START)))
#define SOCK_MARK_WAN_PPPOA_INDEX_TO_MARK(WAN_INDEX)                ((WAN_INDEX << SOCK_MARK_WAN_INDEX_START)+(SOCK_MARK_WAN_INDEX_BIT_MASK - ((MAX_PPTP_NUM+MAX_L2TP_NUM+MAX_VXLAN_NUM+MAX_PPPOA_NUM) << SOCK_MARK_WAN_INDEX_START)))
#define SOCK_MARK_WAN_PPPOE_INDEX_TO_MARK(WAN_INDEX)                ((WAN_INDEX << SOCK_MARK_WAN_INDEX_START)+(SOCK_MARK_WAN_INDEX_BIT_MASK - ((MAX_PPTP_NUM+MAX_L2TP_NUM+MAX_VXLAN_NUM+MAX_PPPOA_NUM+MAX_PPPOE_NUM) << SOCK_MARK_WAN_INDEX_START)))
#define SOCK_MARK_WAN_ATM_INDEX_TO_MARK(WAN_INDEX)                  ((WAN_INDEX << SOCK_MARK_WAN_INDEX_START)+(SOCK_MARK_WAN_INDEX_BIT_MASK - ((MAX_PPTP_NUM+MAX_L2TP_NUM+MAX_VXLAN_NUM+MAX_PPPOA_NUM+MAX_PPPOE_NUM+MAX_ATM_NUM) << SOCK_MARK_WAN_INDEX_START)))
#define SOCK_MARK_WAN_PTM_INDEX_TO_MARK(WAN_INDEX)                  ((WAN_INDEX << SOCK_MARK_WAN_INDEX_START)+(SOCK_MARK_WAN_INDEX_BIT_MASK - ((MAX_PPTP_NUM+MAX_L2TP_NUM+MAX_VXLAN_NUM+MAX_PPPOA_NUM+MAX_PPPOE_NUM+MAX_ATM_NUM+MAX_PTM_NUM) << SOCK_MARK_WAN_INDEX_START)))
#define SOCK_MARK_WAN_ETH_INDEX_TO_MARK(WAN_INDEX)                  ((WAN_INDEX << SOCK_MARK_WAN_INDEX_START)+(SOCK_MARK_WAN_INDEX_BIT_MASK - ((MAX_PPTP_NUM+MAX_L2TP_NUM+MAX_VXLAN_NUM+MAX_PPPOA_NUM+MAX_PPPOE_NUM+MAX_ATM_NUM+MAX_PTM_NUM+MAX_EWAN_NUM) << SOCK_MARK_WAN_INDEX_START)))
#endif

/* MAPE local route: mark[19:19] */
#define SOCK_MARK_MAPE_LOCAL_ROUTE_START                            19
#define SOCK_MARK_MAPE_LOCAL_ROUTE_END                              19
#define SOCK_MARK_MAPE_LOCAL_ROUTE_BIT_NUM                          (SOCK_MARK_MAPE_LOCAL_ROUTE_END-SOCK_MARK_MAPE_LOCAL_ROUTE_START+1)
#define SOCK_MARK_MAPE_LOCAL_ROUTE_BIT_MASK                         (((1<<SOCK_MARK_MAPE_LOCAL_ROUTE_BIT_NUM)-1) << SOCK_MARK_MAPE_LOCAL_ROUTE_START)
#define SOCK_MARK_MAPE_LOCAL_ROUTE_GET(MARK)                        ((MARK & SOCK_MARK_MAPE_LOCAL_ROUTE_BIT_MASK) >> SOCK_MARK_MAPE_LOCAL_ROUTE_START)
#define SOCK_MARK_MAPE_LOCAL_ROUTE_SET(MARK, V)                     ((MARK & ~SOCK_MARK_MAPE_LOCAL_ROUTE_BIT_MASK) | (V << SOCK_MARK_MAPE_LOCAL_ROUTE_START))

/* from LAN: mark[20:20] */
#define SOCK_FROM_LAN_START                                          20
#define SOCK_FROM_LAN_END                                            20
#define SOCK_FROM_LAN_BIT_NUM                                        (SOCK_FROM_LAN_END-SOCK_FROM_LAN_START+1)
#define SOCK_FROM_LAN_BIT_MASK                                       (((1<<SOCK_FROM_LAN_BIT_NUM)-1) << SOCK_FROM_LAN_START)
#define SOCK_FROM_LAN_GET(MARK)                                      ((MARK & SOCK_FROM_LAN_BIT_MASK) >> SOCK_FROM_LAN_START)
#define SOCK_FROM_LAN_SET(MARK, V)                                   ((MARK & ~SOCK_FROM_LAN_BIT_MASK) | (V << SOCK_FROM_LAN_START))

#ifdef CONFIG_YUEME
/* from APPROUTE: mark[21:21] */
#define SOCK_MARK_APPROUTE_ACT_START                                 21
#define SOCK_MARK_APPROUTE_ACT_END                                   21
#define SOCK_MARK_APPROUTE_ACT_BIT_NUM                               (SOCK_MARK_APPROUTE_ACT_END-SOCK_MARK_APPROUTE_ACT_START+1)
#define SOCK_MARK_APPROUTE_ACT_BIT_MASK                              (((1<<SOCK_MARK_APPROUTE_ACT_BIT_NUM)-1) << SOCK_MARK_APPROUTE_ACT_START)
#define SOCK_MARK_APPROUTE_ACT_GET(MARK)                             ((MARK & SOCK_MARK_APPROUTE_ACT_BIT_MASK) >> SOCK_MARK_APPROUTE_ACT_START)
#define SOCK_MARK_APPROUTE_ACT_SET(MARK, V)                          ((MARK & ~SOCK_MARK_APPROUTE_ACT_BIT_MASK) | (V << SOCK_MARK_APPROUTE_ACT_START))
#endif 

/* Remote Access Bit: mark[22:22] */
#define SOCK_MARK_RMACC_START                                        22
#define SOCK_MARK_RMACC_END                                          22
#define SOCK_MARK_RMACC_BIT_NUM                                      (SOCK_MARK_RMACC_END-SOCK_MARK_RMACC_START+1)
#define SOCK_MARK_RMACC_BIT_MASK                                     (((1<<SOCK_MARK_RMACC_BIT_NUM)-1) << SOCK_MARK_RMACC_START)
#define SOCK_MARK_RMACC_GET(MARK)                                    ((MARK & SOCK_MARK_RMACC_BIT_MASK) >> SOCK_MARK_RMACC_START)
#define SOCK_MARK_RMACC_SET(MARK, V)                                 ((MARK & ~SOCK_MARK_RMACC_BIT_MASK) | (V << SOCK_MARK_RMACC_START))
#define RMACC_MARK                                                   "0x400000/0x400000"

/* Reserved: mark[25:23] */

/* Meter index enabled: mark[26:26] */
#define SOCK_MARK_METER_INDEX_ENABLE_START                          26
#define SOCK_MARK_METER_INDEX_ENABLE_END                            26
#define SOCK_MARK_METER_INDEX_ENABLE_BIT_NUM                        (SOCK_MARK_METER_INDEX_ENABLE_END-SOCK_MARK_METER_INDEX_ENABLE_START+1)
#define SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK                       (((1<<SOCK_MARK_METER_INDEX_ENABLE_BIT_NUM)-1) << SOCK_MARK_METER_INDEX_ENABLE_START)
#define SOCK_MARK_MARK_TO_METER_INDEX_ENABLE(MARK)                  ((MARK & SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK) >> SOCK_MARK_METER_INDEX_ENABLE_START)
#define SOCK_MARK_METER_INDEX_ENABLE_TO_MARK(MARK)                  (MARK << SOCK_MARK_METER_INDEX_ENABLE_START)
#define SOCK_MARK_METER_INDEX_ENABLE_GET(MARK)                      ((MARK & SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK) >> SOCK_MARK_METER_INDEX_ENABLE_START)
#define SOCK_MARK_METER_INDEX_ENABLE_SET(MARK, V)                   ((MARK & ~SOCK_MARK_METER_INDEX_ENABLE_BIT_MASK) | (V << SOCK_MARK_METER_INDEX_ENABLE_START))

/* Meter index: mark[31:27] */
#define SOCK_MARK_METER_INDEX_START                                 27
#define SOCK_MARK_METER_INDEX_END                                   31
#define SOCK_MARK_METER_INDEX_BIT_NUM                               (SOCK_MARK_METER_INDEX_END-SOCK_MARK_METER_INDEX_START+1)
#define SOCK_MARK_METER_INDEX_BIT_MASK                              (((1<<SOCK_MARK_METER_INDEX_BIT_NUM)-1) << SOCK_MARK_METER_INDEX_START)
#define SOCK_MARK_MARK_TO_METER_INDEX(MARK)                         ((ARK & SOCK_MARK_METER_INDEX_BIT_MASK) >> SOCK_MARK_METER_INDEX_START)
#define SOCK_MARK_METER_INDEX_TO_MARK(METER_INDEX)                  (METER_INDEX << SOCK_MARK_METER_INDEX_START)

#define SOCK_MARK_QOS_MASK_ALL                                      (SOCK_MARK_8021P_BIT_MASK|SOCK_MARK_8021P_ENABLE_BIT_MASK|SOCK_MARK_QOS_SWQID_BIT_MASK|SOCK_MARK_QOS_ENTRY_BIT_MASK)

#if 1//def CONFIG_RTK_SKB_MARK2
/* [Mark2] Forward by PS: mark2[0:0] */
#define SOCK_MARK2_FORWARD_BY_PS_START                              0
#define SOCK_MARK2_FORWARD_BY_PS_END                                0
#define SOCK_MARK2_FORWARD_BY_PS_BIT_NUM                            (SOCK_MARK2_FORWARD_BY_PS_END-SOCK_MARK2_FORWARD_BY_PS_START+1)
#define SOCK_MARK2_FORWARD_BY_PS_BIT_MASK                           (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_FORWARD_BY_PS_BIT_NUM)-1) << SOCK_MARK2_FORWARD_BY_PS_START)
#define SOCK_MARK2_MARK_TO_FORWARD_BY_PS(MARK)                      (((unsigned long long)MARK & SOCK_MARK2_FORWARD_BY_PS_BIT_MASK) >> SOCK_MARK2_FORWARD_BY_PS_START)
#define SOCK_MARK2_FORWARD_BY_PS_TO_MARK(MARK)                      (unsigned long long)((unsigned long long)MARK << SOCK_MARK2_FORWARD_BY_PS_START)
#define SOCK_MARK2_FORWARD_BY_PS_GET(MARK)                          (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_FORWARD_BY_PS_BIT_MASK) >> SOCK_MARK2_FORWARD_BY_PS_START)
#define SOCK_MARK2_FORWARD_BY_PS_SET(MARK, V)                       (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_FORWARD_BY_PS_BIT_MASK) | ((unsigned long long)V << SOCK_MARK2_FORWARD_BY_PS_START))

/* HW meter enable: mark2[1:1] */
#define SOCK_MARK2_METER_HW_INDEX_ENABLE_START                      1
#define SOCK_MARK2_METER_HW_INDEX_ENABLE_END                        1
#define SOCK_MARK2_METER_HW_INDEX_ENABLE_BIT_NUM                    (SOCK_MARK2_METER_HW_INDEX_ENABLE_END-SOCK_MARK2_METER_HW_INDEX_ENABLE_START+1)
#define SOCK_MARK2_METER_HW_INDEX_ENABLE_BIT_MASK                   (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_METER_HW_INDEX_ENABLE_BIT_NUM)-1) << SOCK_MARK2_METER_HW_INDEX_ENABLE_START)
#define SOCK_MARK2_MARK_TO_METER_HW_INDEX_ENABLE(MARK)              (((unsigned long long)MARK & SOCK_MARK2_METER_HW_INDEX_ENABLE_BIT_MASK) >> SOCK_MARK2_METER_HW_INDEX_ENABLE_START)
#define SOCK_MARK2_METER_HW_INDEX_ENABLE_TO_MARK(MARK)              (unsigned long long)((unsigned long long)MARK << SOCK_MARK2_METER_HW_INDEX_ENABLE_START)
#define SOCK_MARK2_METER_HW_INDEX_ENABLE_GET(MARK)                  (((unsigned long long)MARK & SOCK_MARK2_METER_HW_INDEX_ENABLE_BIT_MASK) >> SOCK_MARK2_METER_HW_INDEX_ENABLE_START)
#define SOCK_MARK2_METER_HW_INDEX_ENABLE_SET(MARK, V)               (((unsigned long long)MARK & ~SOCK_MARK2_METER_HW_INDEX_ENABLE_BIT_MASK) | ((unsigned long long)V << SOCK_MARK2_METER_HW_INDEX_ENABLE_START))

/* MIB counter index enable: mark2[2:2] */
#define SOCK_MARK2_MIB_INDEX_ENABLE_START                           2
#define SOCK_MARK2_MIB_INDEX_ENABLE_END                             2
#define SOCK_MARK2_MIB_INDEX_ENABLE_BIT_NUM                         (SOCK_MARK2_MIB_INDEX_ENABLE_END-SOCK_MARK2_MIB_INDEX_ENABLE_START+1)
#define SOCK_MARK2_MIB_INDEX_ENABLE_BIT_MASK                        ((((unsigned long long)1<<SOCK_MARK2_MIB_INDEX_ENABLE_BIT_NUM)-1) << SOCK_MARK2_MIB_INDEX_ENABLE_START)
#define SOCK_MARK2_MARK_TO_MIB_INDEX_ENABLE(MARK)                   (((unsigned long long)MARK & SOCK_MARK2_MIB_INDEX_ENABLE_BIT_MASK) >> SOCK_MARK2_MIB_INDEX_ENABLE_START)
#define SOCK_MARK2_MIB_INDEX_ENABLE_TO_MARK(MIB_INDEX_ENABLE)       (unsigned long long)((unsigned long long)MIB_INDEX_ENABLE << SOCK_MARK2_MIB_INDEX_ENABLE_START)
#define SOCK_MARK2_METER_HW_INDEX_GET(MARK)                         (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_METER_HW_INDEX_ENABLE_BIT_MASK) >> SOCK_MARK2_METER_HW_INDEX_ENABLE_START)
#define SOCK_MARK2_METER_HW_INDEX_SET(MARK, V)                      (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_METER_HW_INDEX_ENABLE_BIT_MASK) | ((unsigned long long)V << SOCK_MARK2_METER_HW_INDEX_ENABLE_START))

/* [Mark2] mib index: mark[7:3] */
#define SOCK_MARK2_MIB_INDEX_START                                  3
#define SOCK_MARK2_MIB_INDEX_END                                    7
#define SOCK_MARK2_MIB_INDEX_BIT_NUM                                (SOCK_MARK2_MIB_INDEX_END-SOCK_MARK2_MIB_INDEX_START+1)
#define SOCK_MARK2_MIB_INDEX_BIT_MASK                               (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_MIB_INDEX_BIT_NUM)-1) << SOCK_MARK2_MIB_INDEX_START)
#define SOCK_MARK2_MARK_TO_MIB_INDEX(MARK)                          (((unsigned long long)MARK & SOCK_MARK2_MIB_INDEX_BIT_MASK) >> SOCK_MARK2_MIB_INDEX_START)
#define SOCK_MARK2_MIB_INDEX_TO_MARK(MIB_INDEX)                     (unsigned long long)((unsigned long long)MIB_INDEX << SOCK_MARK2_MIB_INDEX_START)
#define SOCK_MARK2_MIB_INDEX_GET(MARK)                              (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_MIB_INDEX_BIT_MASK) >> SOCK_MARK2_MIB_INDEX_START)
#define SOCK_MARK2_MIB_INDEX_SET(MARK, V)                           (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_MIB_INDEX_BIT_MASK) | ((unsigned long long)V << SOCK_MARK2_MIB_INDEX_START))

#define SOCK_MARK2_MIB_MASK_ALL                                     (unsigned long long)(SOCK_MARK2_MIB_INDEX_ENABLE_BIT_MASK|SOCK_MARK2_MIB_INDEX_BIT_MASK|SOCK_MARK2_METER_HW_INDEX_ENABLE_BIT_MASK)

#if defined(CONFIG_RTK_DEV_AP) && defined(CONFIG_USER_AWIFI_SUPPORT)
/* [Mark2] wifidog: mark2[12:8] */
#define SOCK_MARK2_RTK_WIFIDOG_START                                8
#define SOCK_MARK2_RTK_WIFIDOG_END                                  12
#define SOCK_MARK2_RTK_WIFIDOG_BIT_NUM                              (SOCK_MARK2_RTK_WIFIDOG_END-SOCK_MARK2_RTK_WIFIDOG_START+1)
#define SOCK_MARK2_RTK_WIFIDOG_BIT_MASK                             (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_RTK_WIFIDOG_BIT_NUM)-1) << SOCK_MARK2_RTK_WIFIDOG_START)
#endif

#if defined(CONFIG_USER_RTK_MULTICAST_VID)
/* [Mark2] igmp/mld vid: mark2[13:13] */
#define SOCK_MARK2_RTK_IGMP_MLD_VID_START                           13
#define SOCK_MARK2_RTK_IGMP_MLD_VID_END                             13
#define SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_NUM                         (SOCK_MARK2_RTK_IGMP_MLD_VID_END - SOCK_MARK2_RTK_IGMP_MLD_VID_START +1)
#define SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK                        (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_NUM)-1) << SOCK_MARK2_RTK_IGMP_MLD_VID_START)
#define SOCK_MARK2_RTK_IGMP_MLD_VID_GET(MARK)                       (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK) >> SOCK_MARK2_RTK_IGMP_MLD_VID_START)
#define SOCK_MARK2_RTK_IGMP_MLD_VID_SET(MARK, V)                    (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_RTK_IGMP_MLD_VID_BIT_MASK) | ((unsigned long long)V << SOCK_MARK2_RTK_IGMP_MLD_VID_START))
#endif

/* [Mark2] urlblock White List: mark[14:14] */
#define SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_START                     14
#define SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_END                       14
#define SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_BIT_NUM                   (SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_END-SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_START+1)
#define SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_BIT_MASK                  (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_BIT_NUM)-1) << SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_START)
#define SOCK_MARK2_MARK_TO_HTTP_HTTPS_ESTABLISHED(MARK)             (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_BIT_MASK) >> SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_START)
#define SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_TO_MARK(MARK)             (unsigned long long)((unsigned long long)MARK << SOCK_MARK2_HTTP_HTTPS_ESTABLISHED_START)

/* [Mark2] stream ID bit [21:15] */
#define SOCK_MARK2_STREAMID_START                                   15
#define SOCK_MARK2_STREAMID_END                                     21
#define SOCK_MARK2_STREAMID_NUM                                     (SOCK_MARK2_STREAMID_END-SOCK_MARK2_STREAMID_START+1)
#define SOCK_MARK2_STREAMID_MASK                                    (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_STREAMID_NUM)-1) << SOCK_MARK2_STREAMID_START)
#define SOCK_MARK2_STREAMID_GET(MARK)                               (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_STREAMID_MASK) >> SOCK_MARK2_STREAMID_START)
#define SOCK_MARK2_STREAMID_SET(MARK, V)                            (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_STREAMID_MASK) | ((unsigned long long)V << SOCK_MARK2_STREAMID_START))

/* [Mark2] stream ID enable bit [22:22] */
#define SOCK_MARK2_STREAMID_ENABLE_START                            22
#define SOCK_MARK2_STREAMID_ENABLE_END                              22
#define SOCK_MARK2_STREAMID_ENABLE_NUM                              (SOCK_MARK2_STREAMID_ENABLE_END-SOCK_MARK2_STREAMID_ENABLE_START+1)
#define SOCK_MARK2_STREAMID_ENABLE_MASK                             (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_STREAMID_ENABLE_NUM)-1) << SOCK_MARK2_STREAMID_ENABLE_START)
#define SOCK_MARK2_STREAMID_ENABLE_GET(MARK)                        (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_STREAMID_ENABLE_MASK) >> SOCK_MARK2_STREAMID_ENABLE_START)
#define SOCK_MARK2_STREAMID_ENABLE_SET(MARK, V)                     (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_STREAMID_ENABLE_MASK) | ((unsigned long long)V << SOCK_MARK2_STREAMID_ENABLE_START))

/* dscp remark: mark2[28:23] */
#define SOCK_MARK2_DSCP_START                                       23
#define SOCK_MARK2_DSCP_END                                         28
#define SOCK_MARK2_DSCP_BIT_NUM                                     (SOCK_MARK2_DSCP_END - SOCK_MARK2_DSCP_START +1)
#define SOCK_MARK2_DSCP_BIT_MASK                                    (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_DSCP_BIT_NUM)-1) << SOCK_MARK2_DSCP_START) 
#define SOCK_MARK2_DSCP_GET(MARK)                                   (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_DSCP_BIT_MASK) >> SOCK_MARK2_DSCP_START)
#define SOCK_MARK2_DSCP_SET(MARK, V)                                (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_DSCP_BIT_MASK) | ((unsigned long long)V << SOCK_MARK2_DSCP_START))

/* dscp remark enable: mark2[29:29]*/
#define SOCK_MARK2_DSCP_REMARK_ENABLE_START                         29
#define SOCK_MARK2_DSCP_REMARK_ENABLE_END                           29
#define SOCK_MARK2_DSCP_REMARK_ENABLE_NUM                           (SOCK_MARK2_DSCP_REMARK_ENABLE_END-SOCK_MARK2_DSCP_REMARK_ENABLE_START+1)
#define SOCK_MARK2_DSCP_REMARK_ENABLE_MASK                          (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_DSCP_REMARK_ENABLE_NUM)-1) << SOCK_MARK2_DSCP_REMARK_ENABLE_START)
#define SOCK_MARK2_DSCP_REMARK_ENABLE_GET(MARK)                     (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_DSCP_REMARK_ENABLE_MASK) >> SOCK_MARK2_DSCP_REMARK_ENABLE_START)
#define SOCK_MARK2_DSCP_REMARK_ENABLE_SET(MARK, V)                  (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_DSCP_REMARK_ENABLE_MASK) | ((unsigned long long)V << SOCK_MARK2_DSCP_REMARK_ENABLE_START))

/* dscp down remark: mark2[35:30] */
#define SOCK_MARK2_DSCP_DOWN_START                                  30
#define SOCK_MARK2_DSCP_DOWN_END                                    35
#define SOCK_MARK2_DSCP_DOWN_BIT_NUM                                (SOCK_MARK2_DSCP_DOWN_END - SOCK_MARK2_DSCP_DOWN_START +1)
#define SOCK_MARK2_DSCP_DOWN_BIT_MASK                               (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_DSCP_DOWN_BIT_NUM)-1) << SOCK_MARK2_DSCP_DOWN_START) 
#define SOCK_MARK2_DSCP_DOWN_GET(MARK)                              (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_DSCP_DOWN_BIT_MASK) >> SOCK_MARK2_DSCP_DOWN_START)
#define SOCK_MARK2_DSCP_DOWN_SET(MARK, V)                           (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_DSCP_DOWN_BIT_MASK) | ((unsigned long long)V << SOCK_MARK2_DSCP_DOWN_START))

/* dscp down remark enable: mark2[36:36]*/
#define SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_START                    36
#define SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_END                      36
#define SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_NUM                      (SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_END-SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_START+1)
#define SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_MASK                     (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_NUM)-1) << SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_START)
#define SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_GET(MARK)                (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_MASK) >> SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_START)
#define SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_SET(MARK, V)             (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_MASK) | ((unsigned long long)V << SOCK_MARK2_DSCP_DOWN_REMARK_ENABLE_START))

#ifdef CONFIG_YUEME
/* Match appRoute/appFilter rule: mark2[37:37]*/
#define SOCK_MARK2_APPROUTE_RULE_MATCH_START                        37
#define SOCK_MARK2_APPROUTE_RULE_MATCH_END                          37
#define SOCK_MARK2_APPROUTE_RULE_MATCH_NUM                          (SOCK_MARK2_APPROUTE_RULE_MATCH_END-SOCK_MARK2_APPROUTE_RULE_MATCH_START+1)
#define SOCK_MARK2_APPROUTE_RULE_MATCH_MASK                         (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_APPROUTE_RULE_MATCH_NUM)-1) << SOCK_MARK2_APPROUTE_RULE_MATCH_START)
#define SOCK_MARK2_APPROUTE_RULE_MATCH_GET(MARK)                    (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_APPROUTE_RULE_MATCH_MASK) >> SOCK_MARK2_APPROUTE_RULE_MATCH_START)
#define SOCK_MARK2_APPROUTE_RULE_MATCH_SET(MARK, V)                 (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_APPROUTE_RULE_MATCH_MASK) | ((unsigned long long)V << SOCK_MARK2_APPROUTE_RULE_MATCH_START))

#define SOCK_MARK2_APPCTL_LOG_FEATURE_START                         38
#define SOCK_MARK2_APPCTL_LOG_FEATURE_END                           40
#define SOCK_MARK2_APPCTL_LOG_FEATURE_NUM                           (SOCK_MARK2_APPCTL_LOG_FEATURE_END-SOCK_MARK2_APPCTL_LOG_FEATURE_START+1)
#define SOCK_MARK2_APPCTL_LOG_FEATURE_MASK                          (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_APPCTL_LOG_FEATURE_NUM)-1) << SOCK_MARK2_APPCTL_LOG_FEATURE_START)
#define SOCK_MARK2_APPCTL_LOG_FEATURE_GET(MARK)                     (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_APPCTL_LOG_FEATURE_MASK) >> SOCK_MARK2_APPCTL_LOG_FEATURE_START)
#define SOCK_MARK2_APPCTL_LOG_FEATURE_SET(MARK, V)                  (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_APPCTL_LOG_FEATURE_MASK) | ((unsigned long long)V << SOCK_MARK2_APPCTL_LOG_FEATURE_START))
#endif

/* external switch physical port ID enable: mark2[56:56] */
#define SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_START             56
#define SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_END               56
#define SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_NUM               (SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_END-SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_START+1)
#define SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_MASK              (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_NUM)-1) << SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_START)
#define SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_GET(MARK)         (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_MASK) >> SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_START)
#define SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_SET(MARK, V)      (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_MASK) | ((unsigned long long)V << SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_ENABLE_START))

/* external switch physical port ID: mark2[57:59] */
#define SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_START                    57
#define SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_END                      59
#define SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_NUM                      (SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_END-SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_START+1)
#define SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_MASK                     (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_NUM)-1) << SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_START)
#define SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_GET(MARK)                (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_MASK) >> SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_START)
#define SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_SET(MARK, V)             (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_MASK) | ((unsigned long long)V << SOCK_MARK2_EXTERNAL_SWITCH_PORT_ID_START))

/* Match MC/BC filter VEIPIF rule: mark2[60:60] */
#define SOCK_MARK2_BCMC_RULE_VEIPIF_MATCH_START                     60
#define SOCK_MARK2_BCMC_RULE_VEIPIF_MATCH_END                       60
#define SOCK_MARK2_BCMC_RULE_VEIPIF_MATCH_NUM                       (SOCK_MARK2_BCMC_RULE_VEIPIF_MATCH_END-SOCK_MARK2_BCMC_RULE_VEIPIF_MATCH_START+1)
#define SOCK_MARK2_BCMC_RULE_VEIPIF_MATCH_MASK                      (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_BCMC_RULE_VEIPIF_MATCH_NUM)-1) << SOCK_MARK2_BCMC_RULE_VEIPIF_MATCH_START)
#define SOCK_MARK2_BCMC_RULE_VEIPIF_MATCH_GET(MARK)                 (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_BCMC_RULE_VEIPIF_MATCH_MASK) >> SOCK_MARK2_BCMC_RULE_VEIPIF_MATCH_START)
#define SOCK_MARK2_BCMC_RULE_VEIPIF_MATCH_SET(MARK, V)              (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_BCMC_RULE_VEIPIF_MATCH_MASK) | ((unsigned long long)V << SOCK_MARK2_BCMC_RULE_VEIPIF_MATCH_START))

/* [Mark2] Match MC/BC filter rule: mark2[61:61] */
#define SOCK_MARK2_BCMC_RULE_MATCH_START                            61
#define SOCK_MARK2_BCMC_RULE_MATCH_END                              61
#define SOCK_MARK2_BCMC_RULE_MATCH_NUM                              (SOCK_MARK2_BCMC_RULE_MATCH_END-SOCK_MARK2_BCMC_RULE_MATCH_START+1)
#define SOCK_MARK2_BCMC_RULE_MATCH_MASK                             (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_BCMC_RULE_MATCH_NUM)-1) << SOCK_MARK2_BCMC_RULE_MATCH_START)
#define SOCK_MARK2_BCMC_RULE_MATCH_GET(MARK)                        (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_BCMC_RULE_MATCH_MASK) >> SOCK_MARK2_BCMC_RULE_MATCH_START)
#define SOCK_MARK2_BCMC_RULE_MATCH_SET(MARK, V)                     (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_BCMC_RULE_MATCH_MASK) | ((unsigned long long)V << SOCK_MARK2_BCMC_RULE_MATCH_START))

/* [Mark2] Local service accelerate bit [62:62] */ // no use 
//#define SOCK_MARK2_LOCAL_SERVICE_ACC_START                          62
//#define SOCK_MARK2_LOCAL_SERVICE_ACC_END                            62
//#define SOCK_MARK2_LOCAL_SERVICE_ACC_NUM                            (SOCK_MARK2_LOCAL_SERVICE_ACC_END-SOCK_MARK2_LOCAL_SERVICE_ACC_START+1)
//#define SOCK_MARK2_LOCAL_SERVICE_ACC_MASK                           (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_LOCAL_SERVICE_ACC_NUM)-1) << SOCK_MARK2_LOCAL_SERVICE_ACC_START)
//#define SOCK_MARK2_LOCAL_SERVICE_ACC_GET(MARK)                      (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_LOCAL_SERVICE_ACC_MASK) >> SOCK_MARK2_LOCAL_SERVICE_ACC_START)
//#define SOCK_MARK2_LOCAL_SERVICE_ACC_SET(MARK, V)                   (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_LOCAL_SERVICE_ACC_MASK) | ((unsigned long long)V << SOCK_MARK2_LOCAL_SERVICE_ACC_START))

/* Skip FC Func: mark2[63:63] */
#define SOCK_MARK2_SKIP_FC_FUNC_START                               63
#define SOCK_MARK2_SKIP_FC_FUNC_END                                 63
#define SOCK_MARK2_SKIP_FC_FUNC_NUM                                 (SOCK_MARK2_SKIP_FC_FUNC_END-SOCK_MARK2_SKIP_FC_FUNC_START+1)
#define SOCK_MARK2_SKIP_FC_FUNC_MASK                                (unsigned long long)((((unsigned long long)1<<SOCK_MARK2_SKIP_FC_FUNC_NUM)-1) << SOCK_MARK2_SKIP_FC_FUNC_START)
#define SOCK_MARK2_SKIP_FC_FUNC_GET(MARK)                           (unsigned long long)(((unsigned long long)MARK & SOCK_MARK2_SKIP_FC_FUNC_MASK) >> SOCK_MARK2_SKIP_FC_FUNC_START)
#define SOCK_MARK2_SKIP_FC_FUNC_SET(MARK, V)                        (unsigned long long)(((unsigned long long)MARK & ~SOCK_MARK2_SKIP_FC_FUNC_MASK) | ((unsigned long long)V << SOCK_MARK2_SKIP_FC_FUNC_START))

#endif //end CONFIG_RTK_SKB_MARK2
#endif // INCLUDE_SOCKMARKDEFINE_H

