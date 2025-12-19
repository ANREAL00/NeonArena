//---------------------------------------------------------------------------
void TGameWorld::SetPlayerInput(uint8_t playerID, const TInputState &input)
{
	if (playerID < PlayerInputs.size())
	{
		PlayerInputs[playerID] = input;
	}
}
//---------------------------------------------------------------------------
