#ifndef CONTRACT
#define CONTRACT
//#pragma execution_character_set("utf-8")
#include <QString>
const char* g_contract = \
"\
一、记账说明\r\n\
（1）参与知产链记账，您需要承诺能够提供稳定有效的记账服务。记账期间您需要保证您的记账节点稳定可靠的运行记账服务，保证网络的畅通。\r\n\
（2）参与知产链记账，您可以获取记账信用累计。知产链在进行记账候选人筛选时，记账信用累计值会是一个重要的参考因素，持续记账时间越长，累计信用值会越高。\r\n\
（3）了解约束记账节点的行为，防止出现恶意攻击行为，我们要求参与记账的节点缴纳记账保证金。记账保证金只是被冻结到您的账户上，只有退出记账后方可使用。\r\n\
（4）当您不再想参与记账活动时，您可以选择退出记账，在一段时间后，您的保证金将被解冻。解冻保证金是通过DPOC机制保证的，解冻保证金的前提是您并没有进行过恶意攻击知产链网络的活动。\r\n\
（5）记账期间，您需要保证您采用的是安全可靠的记账程序，我们会对恶意攻击知产链网络的记账行为进行严厉惩罚。\r\n\
（6）记账期间，如果由于您的网络或者用于记账的电脑不稳定，导致记账中断（比如：电脑掉电、宕机、网络中断等情况），每次我们都会扣除您的信用积分，直到您被退出记账列表。\r\n\
（7）参与记账后，您的记账过程被严格记录在区块链内，可以通过知产链浏览器，输入您的记账地址来查看您的记账情况。\r\n\
\r\n\
二、记账要求、过程及处罚\r\n\
\r\n\
1、记账要求：\r\n\
（1）要参与记账，首先需要具有如下的硬件要求：\r\n\
        主机：CPU（双核2.5G+）  4G+ Memory   500G+ Harddisk\r\n\
        网络：20M+ bps\r\n\
        运行条件：（MTBF > 100000 小时）\r\n\
（2）缴纳足额保证金（保证金只是被冻结在您的账户上，并不转移给其他地址）\r\n\
（3）建议参与记账者定期将记账收入转移到其他地址上。（这主要为了防止大量零钱的存在，引起后期转账交易的不便）\r\n\
记账过程：\r\n\
（1）加入记账： 缴纳足额保证金并提出记账申请，一段时间后，被接纳为记账成员，加入记账。\r\n\
（2）退出记账：提出退出记账申请，一段时间后，被共识成员会议同意退出记账列表，并在一段时间后，记账保证金解冻。\r\n\
记账处罚：\r\n\
（1）普通处罚：连续 3 轮会议以上都没有记账的节点，系统认为是节点出现故障，会被退出记账列表，保证金自然解冻。\r\n\
（2）严重处罚：节点有恶意攻击网络的行为（如：违规交易被打包、恶意打包等），将会收到严重惩罚。严重惩罚会被退出记账列表，计入黑名单。\r\n\
\r\n\
2、记账保证金的计算\r\n\
\r\n\
记账保证金\r\n\
    记账保证金按照阶梯进行，根据记账总人数确定后续加入的记账节点所需支付的押金。\r\n\
      \r\n\
    记账人数	记账保证金\r\n\
        1~20                 10000  IPC\r\n\
        21~30               20000  IPC\r\n\
        31~40               30000  IPC\r\n\
        41~50               50000  IPC\r\n\
        51~60               80000  IPC\r\n\
        61~70               130000 IPC\r\n\
        71~100             210000 IPC\r\n\
        >100                340000 IPC\r\n\
    \r\n\
3、记账奖励\r\n\
        记账奖励按照如下规则执行：\r\n\
        （1）每年自然增发 960000 （96万）个 IPC \r\n\
        （2）前五年挖矿奖励为 9600000（960万）个 IPC，每年递减。\r\n\
                      第一年：3200000（320万）个IPC\r\n\
                      第二年：2560000（256万）个IPC\r\n\
                      第三年：1920000（192万）个IPC\r\n\
                      第四年：1280000（128万）个IPC\r\n\
                      第五年：640000（64万）个IPC\r\n\
                      第六年：0 个IPC\r\n\
   举例：\r\n\
      按照目前正常的 15S 一个区块的记账速度，全年共 2102400 个 Block，\r\n\
      对于第一年来说：全年每 Block 奖励 = (960000 + 3200000)/2102400 ≈  2\r\n\
      对于第二年来说：全年每 Block 奖励 = (960000 + 2560000)/2102400 ≈  1.7\r\n\
                        \r\n\
三、贡献值（Contribution）计算和使用\r\n\
\r\n\
1、贡献值的计算\r\n\
     贡献值按照记账的次数和被惩罚的情况进行计算。\r\n\
    （1）每成功记账 1 次， Contribution  +1\r\n\
    （2）连续不打包退出记账，Contribution  - 100 \r\n\
    （3）被严重惩罚后，Contribution 归 0，同时被加入黑名单，禁止再次记账\r\n\
    （4）被普通惩罚后，Contribution 保留记录，作为再次记账时抵扣使用\r\n\
\r\n\
2、贡献值的使用（记账保证金抵扣使用）\r\n\
    贡献值使用于申请记账时的基础保证金的抵扣，\r\n\
    （1）贡献值按照千位取整后，可以抵扣记账保证金。（如：原有  8917 分的贡献值，按千位取整后为 8000 的保证金抵扣。此时如果记账保证金要求为 20000 IPC，则保证金可以折算为 12000 IPC。）\r\n\
    （2）贡献值抵扣部分如果到达基础保证金部分，则不再继续抵扣。（如：原有 18917 的贡献值，按千位取整后为 18000 的保证金抵扣。此时如果记账保证金要求为 20000 IPC，则经过抵扣后保证金为 10000 IPC，原因在于基础保证金为 10000 IPC）\r\n\
";
const char* g_clause = \
"\
（1）运行安全 ： 参与DPOC 记账的节点在记账期间，有责任必须保证记账节点（计算机）全天 24 小时不能断网断电，保持持续运行。\r\n\
（2）账号安全 ： 参与 DPOC 记账的节点有责任严格保护好自己参与记账地址的密钥信息，如果密钥信息一旦泄露，请尽快退出 DPOC 记账，并在获取押金后尽快将原地址上的退款转移到其他安全地址上。\r\n\
（3）网络安全 : 参与 DPOC 记账的节点有可能受到来自网络的欺诈攻击，参与记账的使用者有责任对参与记账的设备（计算机）做好安全防范工作。需要开启防火墙和安全实时杀毒工具，防止收到来自网络的攻击，进而成为被控制的攻击者。\r\n\
（4）定期维护 : 参与 DPOC 记账的节点设备（计算机）有可能会由于其他各种原因（如：持续性的断电、网络中断或其他人为因素的关机或关闭节点应用等行为）造成记账节点中断。因此使用者有责任定期关注记账过程是否还在进行中。\r\n\
";
const char* g_contract_en = \
"\
        1. Accounting instructions\r\n\
\r\n\
        （1）Participate in the intellectual property chain bookkeeping, you need to promise to provide stable and effective billing services. During the accounting period, you need to ensure that your accounting nodes run stable and reliable accounting services to ensure the smooth flow of the network.\r\n\
        （2）participate in intellectual property chain account, you can get credit accruals. When the chain of production is being filtered by a bookkeeping candidate, the cumulative value of bookkeeping credits can be an important reference factor. The longer the bookkeeping time, the higher the cumulative credit value.\r\n\
        （3）Constrain the behavior of accounting nodes to prevent malicious attacks, we require the participating nodes to pay deposit. Account security deposit is only frozen to your account, only to withdraw from the account before use.\r\n\
        （4）When you no longer want to participate in accounting activities, you can choose to withdraw from the accounting, after a period of time, your margin will be thawed. The thawing margin is guaranteed through the DPOC mechanism, and the pre-requisite is that you have not carried out any malicious attacks on the production chain network.\r\n\
        （5）During the accounting period, you need to ensure that you are using a safe and secure billing process, and we will severely punish the accounting activities that maliciously attack the network.\r\n\
        （6）During the accounting period, if the account is interrupted due to the instability of your network or the computer used for accounting (such as computer power loss, downtime, network interruption, etc.), we will deduct your credit every time Points until you are signed out of the bookkeeping list.\r\n\
        （7）After you participate in accounting, your accounting process is strictly recorded in the blockchain. You can check your accounting status by entering your accounting address through the browser of the intellectual property browser.\r\n\
\r\n\
\r\n\
        2. Accounting requirements, processes and penalties\r\n\
\r\n\
        2.1 Accounting requirements:\r\n\
        （1）To participate in the accounting, first need to have the following hardware requirements:\r\n\
          —— Host: CPU (dual-core) 2.5G+/ Memory 4G + / Harddisk 500G +\r\n\
          —— Network: 20M + bps\r\n\
          —— Operating conditions: (MTBF> 100,000 hours)\r\n\
        （2）To pay a full deposit (margin is only frozen in your account, not transferred to other addresses)\r\n\
        （3）It is recommended that accountants regularly transfer their accounting income to other addresses. (This is mainly to prevent the existence of a lot of change, causing the inconvenience of late transfer transactions)\r\n\
\r\n\
        2.2 Accounting process：\r\n\
        （1）To join the accounting: to pay a full deposit and make an accounting application, after a period of time, was admitted as a member of accounting, to join the accounting.\r\n\
        （2）Exit accounting: proposed withdrawal of accounting applications, after a period of time, the consent of members of the meeting agreed to withdraw from the accounting list, and after some time, accounting margin thaw.\r\n\
\r\n\
        2.3 Accounting penalties：\r\n\
        （1）Ordinary Penalty: Nodes that have not been booked for more than three consecutive sessions are considered as node failures, they will be logged out of the accounting list, and the margin will naturally thaw.\r\n\
        （2）Serious Penalty: Nodes will maliciously attack the network (such as illegal trading is packaged, malicious packaging, etc.), will receive serious penalties. Serious penalties will be withdrawn from the billing list and credited to the blacklist.\r\n\
\r\n\
\r\n\
        3. Accounting security deposit calculation\r\n\
\r\n\
        3.1 Accounting security deposit(Testnet)\r\n\
                Deposit according to the ladder, according to the total number of accounts to determine the follow-up to join the accounting node to pay the deposit\r\n\
\r\n\
        Bookkeeping number	Deposit\r\n\
             1~20                 10000  IPC\r\n\
             21~30               20000  IPC\r\n\
             31~40               30000  IPC\r\n\
             41~50               50000  IPC\r\n\
             51~60               80000  IPC\r\n\
             61~70               130000 IPC\r\n\
             71~100             210000 IPC\r\n\
             >100                340000 IPC\r\n\
             \r\n\
\r\n\
        3.2 Book rewards：\r\n\
                Book rewards in accordance with the following rules：\r\n\
                （1）Each year a natural increase of 960000 (960000) IPC\r\n\
                （2）The first five years mining reward 9600000 (9.6 million) IPC, decreasing each year.\r\n\
\r\n\
        First year: 3200000 (3.2 million) IPC\r\n\
        The second year: 2560000 (2560000) IPC\r\n\
        Third year: 1920000 (1.92 million) IPC\r\n\
        Fourth year: 1280000 (1280000) IPC\r\n\
        Fifth year: 640,000 (640,000) IPC\r\n\
        Sixth year: 0 IPC\r\n\
\r\n\
        Example:\r\n\
               In accordance with the normal 15S a block accounting speed, a total of 2102400 Block：For the first year: Annual bonus per unit = (960000 + 3200000) / 2102400 ≈ 2.   For the second year: Annual bonus per unit = (960000 + 2560000) / 2102400 ≈ 1.7\r\n\
\r\n\
        4. Contribution value calculation and use\r\n\
        4.1 Contribution value calculation:\r\n\
              Contributions are calculated based on the number of records and penalties.\r\n\
                 (1) Once per successful accounting, Contribution +1\r\n\
                 (2) Unbundled out of account, Contribution - 100\r\n\
                 (3) was severely punished, Contribution to 0, while being added to the blacklist, prohibit re-bookkeeping\r\n\
                 (4) After being punished normally, Contribution keeps the record and deducts it for re-bookkeeping\r\n\
\r\n\
        4.2 Use of Contribution Value (Credits Deposit Deductible Use)\r\n\
                The contribution value is used to deduct the basic deposit when applying for bookkeeping.\r\n\
        (1)	The contribution value can be deducted from the book deposit after it is rounded to the nearest thousand. (For example, the contribution of the original 8917 points is deducted by a deposit of 8000 after taking a thousand positions.) At this time if the deposit deposit requirement is 20000 IPC, the deposit can be converted into 12000 IPC.)\r\n\
\r\n\
        (2)	Contribution Value Deductible If the basic deposit is reached, the deductible will not be continued. (For example, the contribution value of the original 18917 is deducted by a deposit of 18000 after taking a thousand positions.) At this time, if the accounting deposit requirement is 20000 IPC, the deductible deposit is 10000 IPC after the base deposit is 10000 IPC)\r\n\
\r\n\
";
const char* g_clause_en = \
"\
（1)  Operational safety: Nodes participating in DPOC accounting are responsible for ensuring that the accounting node (computer) can not be powered off and on for 24 hours a day during the accounting period, keeping it operating continuously.\r\n\
（2） Account Security: The node participating in DPOC accounting has the responsibility to strictly protect the key information that participates in the accounting address. If the key information is leaked, please withdraw from DPOC accounting as soon as possible and return the deposit on the original address as soon as possible.\r\n\
（3） Cyber security: Nodes participating in DPOC accounting may be subject to fraudulent attacks from the network, and it is the user's responsibility to take precautions against the devices (computers) involved in accounting. Need to open the firewall and real-time anti-virus security tools to prevent the receipt of attacks from the network, and then become controlled by the attacker.\r\n\
（4） Periodic Maintenance: Node devices (computers) participating in DPOC accounting may cause accounting node outages due to various other reasons (such as persistent power outage, network interruption or other human factor shutdown or node application off) . It is therefore the responsibility of the user to pay regular attention to whether the accounting process is still in progress.\r\n\
";
#endif // CONTRACT

