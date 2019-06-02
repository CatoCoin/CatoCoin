CatoCoin - Port: 33888 RPC Port: 6082

Updated : 6/2/2019

Specs
  Type: Masternode/POS
  Tiered Reward Split: 
	Tier 1: 10% MN 
	Tier 2: 25% MN 
	Tier 3: 60% MN
	POS:    5%
  Block Time: 60 Seconds
  Confirmations: 15
  Masternode Confirmations: 15
  Minimum coins required for staking: 100
  POS Maturity Time: 2 hours
  Total Coin Supply: 42,000,000
  Pre mine: 400,000
  Pre-mine %: Less than 1%

Source code updated for planned Fork at block: 495,217 where all running masternoedes are de-activated and new Collateral requirements go into effect. Please see: https://discordapp.com/channels/452458016617267201/464028489939550208/580314958173175809 for new collateral requirements

You do NOT need to re-index any files for the fork. Insure all of your wallets are updated (linux & windows) before block 495,217



Linux Compiling Instruction
  ./autogen.sh
  ./configure --disable-tests --disable-bench
  make

when done
  strip ./catocoind
  strip ./catcoin-cli
