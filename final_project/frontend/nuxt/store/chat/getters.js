export default {
  getParticipantById: (state) => (id) => {
    let currParticipant
    state.participants.forEach((participant) => {
      if (participant.id === id) {
        currParticipant = participant
      }
    })
    return currParticipant
  },
  messages: (state) => {
    const messages = []
    state.messages.forEach((message) => {
      const newMessage = { ...message }
      messages.push(newMessage)
    })
    return messages
  },
  myself: (state) => {
    return state.myself
  }
}
