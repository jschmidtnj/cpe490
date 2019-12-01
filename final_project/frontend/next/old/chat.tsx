import React from 'react'
import { Component } from 'react'
import axios from 'axios'
import { Widget, addResponseMessage, dropMessages } from 'react-chat-widget'
import Select from 'react-select'
import 'react-chat-widget/lib/styles.css'
import config from '../assets/config'
import { toast } from 'react-toastify'

let emergencyOptions = []

for (let i = 0; i < config.emergencyServicesAddresses.length; i++) {
  emergencyOptions.push({
    label: `emergency services ${i + 1}`,
    value: config.emergencyServicesAddresses[i]
  })
}

const messageType = '\x00'

type optionType = {
  label: string,
  value: string
}

type stateType = {
  socket: WebSocket,
  organizer: boolean,
  source: string,
  destination: optionType,
  chatOptions: Array<optionType>
}

class Chat extends Component<{}, stateType> {
  constructor(props) {
    super(props)
    this.state = {
      socket: null,
      organizer: false,
      source: '',
      destination: null,
      chatOptions: []
    }
  }

  componentWillUnmount() {
    if (this.state.socket) {
      this.state.socket.close()
    }
    dropMessages()
  }

  updateChatSelection() {
    axios.get(`http://${config.apiurl}${config.senderListEndpoint}`).then(res => {
      if (res.status == 200 && res.data.hasOwnProperty('senders')) {
        let senders = []
        for (let i = 0; i < res.data['senders'].length; i++) {
          senders.push({
            value: String.fromCharCode(res.data['senders'][i]),
            label: `emergency ${i + 1}`
          })
        }
        this.setState(prevState => ({
          ...prevState,
          chatOptions: senders
        }))
      } else {
        toast.warn('no senders found')
      }
    }).catch(err => {
      toast.error('got error with get senders request', err)
    })
  }

  componentDidMount() {
    axios.get(`http://${config.apiurl}${config.configEndpoint}`).then(res => {
      if (res.status == 200 && res.data.hasOwnProperty('source')) {
        if (emergencyOptions.find(option => option.value.charCodeAt(0) == res.data['source'])) {
          this.setState(prevState => ({
            ...prevState,
            organizer: true
          }))
          // the current node is the emergency worker
          this.updateChatSelection()
        } else {
          // the current node is a person who needs help
          this.setState(prevState => ({
            ...prevState,
            destination: emergencyOptions[0]
          }))
        }
        this.setState(prevState => ({
          ...prevState,
          socket: new window.WebSocket(`ws://${config.apiurl}${config.websocketEndpoint}`),
          source: res.data['source']
        }), () => {
          this.state.socket.onopen = evt => {
            toast.success('Connection established')
          }
          this.state.socket.onmessage = evt => {
            // console.log(`[message] Data received from server: ${evt.data}`)
            let jsonData
            try {
              jsonData = JSON.parse(evt.data)
            } catch (err) {
              toast.error(err)
            }
            if (jsonData.hasOwnProperty('debug')) {
              console.log(jsonData['debug'])
            } else if (jsonData.hasOwnProperty('message')) {
              addResponseMessage(jsonData['message'])
            }
          }
          this.state.socket.onclose = evt => {
            if (evt.wasClean) {
              console.log(`[close] Connection closed cleanly, code=${evt.code} reason=${evt.reason}`)
            } else {
              toast.error('[close] Connection died')
            }
          }
          this.state.socket.onerror = err => {
            console.error(`[error]: `, err)
          }
        })
      } else {
        toast.warn('no source found')
      }
    }).catch(err => {
      const message = 'got error with get config request'
      toast.error(message)
      console.log(message, err)
    })
  }

  handleNewUserMessage = newMessage => {
    console.log(`New message sending: ${newMessage}`)
    if (this.state.destination) {
      newMessage = this.state.destination.value + messageType + newMessage
      this.state.socket.send(newMessage)
    } else {
      toast.error('no destination found')
    }
  }

  handleChatSelect = destination => {
    this.setState(prevState => ({
      ...prevState,
      destination: destination
    }))
    dropMessages()
  }

  chatSelecter() {
    if (!this.state.organizer)
      return
    return <Select
      value={this.state.destination}
      onChange={this.handleChatSelect}
      options={this.state.chatOptions}
    />
  }

  render() {
    return (
      <div>
        {this.chatSelecter()}
        <Widget
          handleNewUserMessage={this.handleNewUserMessage}
          title="Emergency services"
          subtitle="chat"
        />
      </div>
    )
  }
}

export default Chat
